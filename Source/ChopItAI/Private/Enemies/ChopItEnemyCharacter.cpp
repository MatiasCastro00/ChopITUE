#include "Enemies/ChopItEnemyCharacter.h"

#include "ChopItCollision.h"
#include "ChopItLogChannels.h"
#include "Combat/ChopItDamageTypes.h"
#include "Combat/ChopItHealthComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/ChopItCameraFacingTextComponent.h"
#include "DrawDebugHelpers.h"
#include "Enemies/ChopItEnemyDefinition.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Harvest/ChopItLogPickup.h"
#include "Feedback/ChopItHitFeedbackComponent.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

AChopItEnemyCharacter::AChopItEnemyCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	GetCapsuleComponent()->InitCapsuleSize(38.0f, 68.0f);
	GetCapsuleComponent()->SetCollisionProfileName(ChopItCollisionProfiles::Enemy);
	// Keep this explicit as well as in DefaultEngine.ini so an old Blueprint CDO
	// or stale profile cache can never make enemies disturb the simulated rope.
	GetCapsuleComponent()->SetCollisionResponseToChannel(ChopItCollisionChannels::Chain, ECR_Ignore);
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->bRunPhysicsWithNoController = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 720.0f, 0.0f);
	GetCharacterMovement()->GravityScale = 1.0f;
	BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BodyMesh"));
	BodyMesh->SetupAttachment(GetCapsuleComponent());
	BodyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BodyMesh->SetRelativeScale3D(FVector(0.72f, 0.72f, 1.2f));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cone(TEXT("/Engine/BasicShapes/Cone.Cone"));
	if (Cone.Succeeded()) { BodyMesh->SetStaticMesh(Cone.Object); }
	Label = CreateDefaultSubobject<UChopItCameraFacingTextComponent>(TEXT("Label"));
	Label->SetupAttachment(GetCapsuleComponent());
	Label->SetRelativeLocation(FVector(0.0f, 0.0f, 105.0f));
	Label->SetRelativeRotation(FRotator(0.0f, 180.0f, 0.0f));
	Label->SetHorizontalAlignment(EHTA_Center);
	Label->SetWorldSize(24.0f);
	Label->SetTextRenderColor(FColor(255, 110, 55));
	HealthComponent = CreateDefaultSubobject<UChopItHealthComponent>(TEXT("HealthComponent"));
	HitFeedbackComponent = CreateDefaultSubobject<UChopItHitFeedbackComponent>(TEXT("HitFeedbackComponent"));
	HitFeedbackComponent->SetVisualComponent(BodyMesh);
	HitFeedbackComponent->SetWoodenTarget(true);
}

void AChopItEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();
	if (UMaterialInterface* EnemyMaterial = LoadObject<UMaterialInterface>(
		nullptr, TEXT("/Game/ChopIt/World/Blockout/Materials/MI_Enemy.MI_Enemy")))
	{
		BodyMesh->SetMaterial(0, EnemyMaterial);
	}
	HealthComponent->OnDeath.AddUObject(this, &AChopItEnemyCharacter::HandleDeath);
	UpdateLabel();
}

void AChopItEnemyCharacter::InitializeFromDefinition(UChopItEnemyDefinition* NewDefinition, AActor* NewTarget)
{
	Definition = NewDefinition;
	TargetActor = NewTarget;
	if (Definition)
	{
		GetCharacterMovement()->MaxWalkSpeed = Definition->MoveSpeed;
		HealthComponent->SetMaxHealth(Definition->MaxHealth);
	}
	UpdateLabel();
}

void AChopItEnemyCharacter::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!Definition || !HealthComponent->IsAlive() || !TargetActor.IsValid()) { return; }
	AActor* Target = TargetActor.Get();
	FVector ToTarget = Target->GetActorLocation() - GetActorLocation();
	ToTarget.Z = 0.0f;
	const float Distance = ToTarget.Size();
	if (Distance > Definition->AttackRange)
	{
		// These lightweight horde actors deliberately have no AIController yet.
		// Force input keeps CharacterMovement active without introducing a nav/BT dependency.
		AddMovementInput(ToTarget.GetSafeNormal(), 1.0f, true);
		return;
	}
	const double Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	if (Now < NextAttackAt) { return; }
	if (UChopItHealthComponent* TargetHealth = Target->FindComponentByClass<UChopItHealthComponent>())
	{
		FChopItDamageSpec Damage;
		Damage.BaseDamage = Definition->ContactDamage;
		TargetHealth->ApplyDamage(Damage, this, Target->GetActorLocation() + FVector(0.0f, 0.0f, 65.0f));
		DrawDebugSphere(GetWorld(), Target->GetActorLocation() + FVector(0, 0, 55), 45.0f, 12, FColor::Red, false, 0.15f, 0, 2.0f);
	}
	NextAttackAt = Now + Definition->AttackInterval;
}

void AChopItEnemyCharacter::HandleDeath(AActor* DeadActor, AActor* DamageSource)
{
	AwardExperience();
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BodyMesh->SetVisibility(false);
	Label->SetText(FText::FromString(TEXT("DEFEATED")));
	SetLifeSpan(0.3f);
	UE_LOG(LogChopIt, Display, TEXT("Enemy %s defeated; XP awarded once."), *GetName());
}

void AChopItEnemyCharacter::AwardExperience()
{
	if (bRewardGranted || !Definition) { return; }
	bRewardGranted = true;
	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (AChopItLogPickup* Log = GetWorld()->SpawnActor<AChopItLogPickup>(
		AChopItLogPickup::StaticClass(), GetActorLocation() + FVector(0.0f, 0.0f, 35.0f), FRotator::ZeroRotator, SpawnParameters))
	{
		Log->InitializeReward(Definition->WoodRewardUnits, Definition->ExperienceReward);
	}
}

void AChopItEnemyCharacter::UpdateLabel() const
{
	if (Label)
	{
		Label->SetText(FText::FromString(Definition ? Definition->DisplayName.ToString() : TEXT("ENEMY")));
	}
}
