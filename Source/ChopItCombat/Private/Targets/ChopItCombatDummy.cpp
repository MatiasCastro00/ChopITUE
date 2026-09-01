#include "Targets/ChopItCombatDummy.h"

#include "ChopItCollision.h"
#include "ChopItLogChannels.h"
#include "Combat/ChopItHealthComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/ChopItCameraFacingTextComponent.h"
#include "UObject/ConstructorHelpers.h"

AChopItCombatDummy::AChopItCombatDummy()
{
	PrimaryActorTick.bCanEverTick = false;

	BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BodyMesh"));
	SetRootComponent(BodyMesh);
	BodyMesh->SetCollisionProfileName(ChopItCollisionProfiles::Enemy);
	BodyMesh->SetRelativeScale3D(FVector(0.8f, 0.8f, 1.5f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (CylinderMesh.Succeeded())
	{
		BodyMesh->SetStaticMesh(CylinderMesh.Object);
	}

	HealthLabel = CreateDefaultSubobject<UChopItCameraFacingTextComponent>(TEXT("HealthLabel"));
	HealthLabel->SetupAttachment(BodyMesh);
	HealthLabel->SetRelativeLocation(FVector(0, 0, 120));
	HealthLabel->SetRelativeRotation(FRotator(0, 180, 0));
	HealthLabel->SetHorizontalAlignment(EHTA_Center);
	HealthLabel->SetWorldSize(42.0f);
	HealthLabel->SetTextRenderColor(FColor::White);

	HealthComponent = CreateDefaultSubobject<UChopItHealthComponent>(TEXT("HealthComponent"));
}

void AChopItCombatDummy::BeginPlay()
{
	Super::BeginPlay();
	HealthComponent->OnHealthChanged.AddUObject(this, &AChopItCombatDummy::HandleHealthChanged);
	HealthComponent->OnDeath.AddUObject(this, &AChopItCombatDummy::HandleDeath);
	UpdateHealthLabel(HealthComponent->GetCurrentHealth(), HealthComponent->GetMaxHealth());
}

void AChopItCombatDummy::HandleHealthChanged(const float CurrentHealth, const float MaxHealth, AActor* DamageSource)
{
	UpdateHealthLabel(CurrentHealth, MaxHealth);
	UE_LOG(
		LogChopIt,
		Display,
		TEXT("Combat dummy %s health: %.0f / %.0f (source: %s)"),
		*GetName(),
		CurrentHealth,
		MaxHealth,
		*GetNameSafe(DamageSource));
}

void AChopItCombatDummy::HandleDeath(AActor* DeadActor, AActor* DamageSource)
{
	UE_LOG(LogChopIt, Display, TEXT("Combat dummy %s defeated exactly once."), *GetName());
	BodyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BodyMesh->SetVisibility(false);
	HealthLabel->SetText(FText::FromString(TEXT("KO")));
	SetLifeSpan(1.0f);
}

void AChopItCombatDummy::UpdateHealthLabel(const float CurrentHealth, const float MaxHealth)
{
	HealthLabel->SetText(FText::FromString(FString::Printf(TEXT("%.0f / %.0f"), CurrentHealth, MaxHealth)));
}
