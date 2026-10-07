#include "Rewards/ChopItBossRewardChest.h"

#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/ChopItCameraFacingTextComponent.h"
#include "Items/ChopItItemComponent.h"
#include "Items/ChopItItemDataAsset.h"
#include "Items/ChopItItemLootSubsystem.h"
#include "Combat/ChopItHealthComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "Framework/ChopItPlayerController.h"
#include "Player/ChopItCharacter.h"
#include "UI/ChopItHUD.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"

AChopItBossRewardChest::AChopItBossRewardChest()
{
	PrimaryActorTick.bCanEverTick = true;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
	VisualRoot = CreateDefaultSubobject<USceneComponent>(TEXT("VisualRoot"));
	VisualRoot->SetupAttachment(Root);
	Approach = CreateDefaultSubobject<USphereComponent>(TEXT("Approach"));
	Approach->SetupAttachment(Root);
	Approach->SetSphereRadius(155.f);
	Approach->SetRelativeLocation(FVector(0.f, 0.f, 55.f));
	Approach->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Approach->SetCollisionResponseToAllChannels(ECR_Ignore);
	Approach->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	Approach->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::HandleApproach);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Wood(TEXT("/Game/ChopIt/Items/Chest/M_BossChest_Wood.M_BossChest_Wood"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Gold(TEXT("/Game/ChopIt/Items/Chest/M_BossChest_Gold.M_BossChest_Gold"));
	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ChestBody"));
	Body->SetupAttachment(VisualRoot);
	Body->SetStaticMesh(Cube.Object);
	Body->SetMaterial(0, Wood.Object);
	Body->SetRelativeLocation(FVector(0.f, 0.f, 34.f));
	Body->SetRelativeScale3D(FVector(1.2f, 0.85f, 0.62f));
	Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	Lid = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ChestLid"));
	Lid->SetupAttachment(VisualRoot);
	Lid->SetStaticMesh(Cube.Object);
	Lid->SetMaterial(0, Gold.Object);
	Lid->SetRelativeLocation(FVector(0.f, 0.f, 73.f));
	Lid->SetRelativeScale3D(FVector(1.3f, 0.95f, 0.2f));
	Lid->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	Lock = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ChestLock"));
	Lock->SetupAttachment(VisualRoot);
	Lock->SetStaticMesh(Cube.Object);
	Lock->SetMaterial(0, Gold.Object);
	Lock->SetRelativeLocation(FVector(0.f, -45.f, 45.f));
	Lock->SetRelativeScale3D(FVector(0.23f, 0.12f, 0.28f));
	Lock->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	Label = CreateDefaultSubobject<UChopItCameraFacingTextComponent>(TEXT("RewardLabel"));
	Label->SetupAttachment(VisualRoot);
	Label->SetRelativeLocation(FVector(0.f, 0.f, 118.f));
	Label->SetHorizontalAlignment(EHTA_Center);
	Label->SetWorldSize(24.f);
	Label->SetTextRenderColor(FColor(255, 204, 82));
	Label->SetText(FText::FromString(TEXT("COFRE DEL BOSS\nACÉRCATE")));

	Light = CreateDefaultSubobject<UPointLightComponent>(TEXT("GoldGlow"));
	Light->SetupAttachment(VisualRoot);
	Light->SetRelativeLocation(FVector(0.f, 0.f, 82.f));
	Light->SetLightColor(FLinearColor(1.f, 0.58f, 0.12f));
	Light->SetIntensity(2000.f);
	Light->SetAttenuationRadius(350.f);
	Light->CastShadows = false;

	ArrivalParticles = CreateDefaultSubobject<UNiagaraComponent>(TEXT("ArrivalParticles"));
	ArrivalParticles->SetupAttachment(Root);
	ArrivalParticles->SetRelativeLocation(FVector(0.f, 0.f, 45.f));
	ArrivalParticles->SetAutoActivate(false);
	OpeningParticles = CreateDefaultSubobject<UNiagaraComponent>(TEXT("OpeningParticles"));
	OpeningParticles->SetupAttachment(VisualRoot);
	OpeningParticles->SetRelativeLocation(FVector(0.f, 0.f, 84.f));
	OpeningParticles->SetAutoActivate(false);
	IdleParticles = CreateDefaultSubobject<UNiagaraComponent>(TEXT("IdleParticles"));
	IdleParticles->SetupAttachment(VisualRoot);
	IdleParticles->SetRelativeLocation(FVector(0.f, 0.f, 58.f));
	IdleParticles->SetAutoActivate(false);
	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> Arrival(TEXT("/Game/ChopIt/Items/Chest/NS_BossChest_Appear.NS_BossChest_Appear"));
	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> Opening(TEXT("/Game/ChopIt/Items/Chest/NS_BossChest_Open.NS_BossChest_Open"));
	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> Idle(TEXT("/Game/ChopIt/Items/Chest/NS_BossChest_Idle.NS_BossChest_Idle"));
	ArrivalParticles->SetAsset(Arrival.Object);
	OpeningParticles->SetAsset(Opening.Object);
	IdleParticles->SetAsset(Idle.Object);
	const FBox ParticleBounds(FVector(-350.f, -350.f, -250.f), FVector(350.f, 350.f, 350.f));
	ArrivalParticles->SetSystemFixedBounds(ParticleBounds);
	OpeningParticles->SetSystemFixedBounds(ParticleBounds);
	IdleParticles->SetSystemFixedBounds(ParticleBounds);
	for (UNiagaraComponent* Component : {ArrivalParticles.Get(), OpeningParticles.Get(), IdleParticles.Get()})
	{
		Component->SetEmitterFixedBounds(TEXT("GoldSparks"), ParticleBounds);
		Component->SetEmitterFixedBounds(TEXT("ForestDust"), ParticleBounds);
	}
}

void AChopItBossRewardChest::BeginPlay()
{
	Super::BeginPlay();
	SetActorScale3D(FVector(0.05f));
	ArrivalParticles->Activate(true);
}

void AChopItBossRewardChest::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!bReady)
	{
		EntranceTime += DeltaSeconds;
		const float T = FMath::Clamp(EntranceTime / 0.7f, 0.f, 1.f);
		const float Overshoot = 1.f + 0.16f * FMath::Sin(T * PI) * (1.f - T);
		SetActorScale3D(FVector(FMath::Lerp(0.05f, 1.f, T) * Overshoot));
		if (T >= 1.f)
		{
			bReady = true;
			Approach->UpdateOverlaps();
			FindNearbyPlayer();
		}
	}
	if (bReady && !bOpening)
	{
		IdleTime += DeltaSeconds;
		IdleBurstTime += DeltaSeconds;
		const float Breath = FMath::Sin(IdleTime * 2.5f);
		VisualRoot->SetRelativeLocation(FVector(0.f, 0.f, 5.f * FMath::Sin(IdleTime * 1.7f)));
		VisualRoot->SetRelativeScale3D(FVector(1.f + 0.025f * Breath));
		Light->SetIntensity(2100.f + 420.f * Breath);
		if (IdleBurstTime >= 2.0f)
		{
			IdleBurstTime = 0.f;
			if (IdleParticles->GetAsset()) IdleParticles->Activate(true);
		}
	}
	if (bOpening && LidTime < 0.65f)
	{
		LidTime = FMath::Min(0.65f, LidTime + DeltaSeconds);
		const float T = 1.f - FMath::Square(1.f - LidTime / 0.65f);
		Lid->SetRelativeLocation(FVector(0.f, 0.f, FMath::Lerp(73.f, 118.f, T)));
		Lid->SetRelativeRotation(FRotator(FMath::Lerp(0.f, -22.f, T), 0.f, 0.f));
		Light->SetIntensity(FMath::Lerp(2000.f, 5200.f, T));
	}
	if (bOpening && LidTime >= 0.65f) SetActorTickEnabled(false);
}

void AChopItBossRewardChest::FindNearbyPlayer()
{
	APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	if (PC && PC->GetPawn() && Approach->IsOverlappingActor(PC->GetPawn())) TryOpen(PC->GetPawn());
}

void AChopItBossRewardChest::HandleApproach(UPrimitiveComponent*, AActor* OtherActor,
	UPrimitiveComponent*, int32, bool, const FHitResult&)
{
	TryOpen(OtherActor);
}

void AChopItBossRewardChest::TryOpen(AActor* Actor)
{
	if (!bReady || bOpening || !Actor || !GetWorld()) return;
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!PC || Actor != PC->GetPawn()) return;
	if (const UChopItHealthComponent* Health = Actor->FindComponentByClass<UChopItHealthComponent>();
		Health && !Health->IsAlive()) return;
	UChopItItemComponent* Inventory = Actor->FindComponentByClass<UChopItItemComponent>();
	UChopItItemLootSubsystem* Loot = GetGameInstance() ? GetGameInstance()->GetSubsystem<UChopItItemLootSubsystem>() : nullptr;
	if (!Inventory || !Loot) return;
	const AChopItCharacter* Character = Cast<AChopItCharacter>(Actor);
	const float LuckPercent = Character ? Character->GetLuckPercent() : 0.f;
	ChosenItem = Loot->GetRandomItemWithLuck(Inventory, LuckPercent);
	if (!ChosenItem) return; // Keep the chest available if content is temporarily unavailable.
	Recipient = Inventory;
	bOpening = true;
	VisualRoot->SetRelativeLocation(FVector::ZeroVector);
	VisualRoot->SetRelativeScale3D(FVector::OneVector);
	IdleParticles->Deactivate();
	Approach->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetActorTickEnabled(true);
	OpeningParticles->Activate(true);
	Label->SetText(FText::FromString(TEXT("ABRIENDO...")));
	if (AChopItHUD* HUD = Cast<AChopItHUD>(PC->GetHUD())) HUD->StartItemReveal(ChosenItem);
	if (USoundBase* Sound = LoadObject<USoundBase>(nullptr, TEXT("/Game/ChopIt/UI/Audio/UI_Complete.UI_Complete")))
		UGameplayStatics::PlaySoundAtLocation(this, Sound, GetActorLocation());
	// Award synchronously because world timers stop during the paused reveal.
	// The HUD keeps the result hidden until the real-time light sequence ends.
	GrantReward();
}

void AChopItBossRewardChest::GrantReward()
{
	if (bGranted || !ChosenItem) return;
	const UChopItHealthComponent* Health = Recipient.IsValid() && Recipient->GetOwner()
		? Recipient->GetOwner()->FindComponentByClass<UChopItHealthComponent>() : nullptr;
	if (!Recipient.IsValid() || (Health && !Health->IsAlive()) || !Recipient->AddItem(ChosenItem))
	{
		// Missing player or rejected inventory write: restore an unclaimed chest.
		if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
			if (AChopItHUD* HUD = Cast<AChopItHUD>(PC->GetHUD())) HUD->CancelItemReveal();
		bOpening = false;
		SetActorTickEnabled(true);
		LidTime = 0.f;
		Lid->SetRelativeLocation(FVector(0.f, 0.f, 73.f));
		Lid->SetRelativeRotation(FRotator::ZeroRotator);
		ChosenItem = nullptr;
		Approach->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		Label->SetText(FText::FromString(TEXT("COFRE DEL BOSS\nACÉRCATE")));
		return;
	}
	bGranted = true;
	Label->SetText(ChosenItem->DisplayName);
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
		if (AChopItHUD* HUD = Cast<AChopItHUD>(PC->GetHUD())) HUD->CompleteItemReveal(ChosenItem);
	SetLifeSpan(4.f);
}

void AChopItBossRewardChest::EndPlay(const EEndPlayReason::Type Reason)
{
	Super::EndPlay(Reason);
}
