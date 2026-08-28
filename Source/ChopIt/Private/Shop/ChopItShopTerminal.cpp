#include "Shop/ChopItShopTerminal.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Framework/ChopItPlayerState.h"
#include "Shop/ChopItShopComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Weapons/ChopItWeaponDefinition.h"

AChopItShopTerminal::AChopItShopTerminal()
{
	PrimaryActorTick.bCanEverTick = false;
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);
	TerminalVisual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TerminalVisual"));
	TerminalVisual->SetupAttachment(SceneRoot);
	TerminalVisual->SetRelativeLocation(FVector(0.0f, 0.0f, 110.0f));
	TerminalVisual->SetRelativeScale3D(FVector(0.9f, 0.9f, 2.2f));
	TerminalVisual->SetCollisionProfileName(TEXT("BlockAll"));
	TerminalLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("TerminalLabel"));
	TerminalLabel->SetupAttachment(SceneRoot);
	TerminalLabel->SetRelativeLocation(FVector(0.0f, 0.0f, 290.0f));
	TerminalLabel->SetRelativeRotation(FRotator(0.0f, 180.0f, 0.0f));
	TerminalLabel->SetHorizontalAlignment(EHTA_Center);
	TerminalLabel->SetWorldSize(34.0f);
	TerminalLabel->SetText(FText::FromString(TEXT("E: TIENDA DE HERRAMIENTAS")));
	TerminalLabel->SetTextRenderColor(FColor::Cyan);
	TerminalLabel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded()) { TerminalVisual->SetStaticMesh(CubeMesh.Object); }
}

void AChopItShopTerminal::BeginPlay()
{
	Super::BeginPlay();
	static const TCHAR* Paths[] =
	{
		TEXT("/Game/ChopIt/Combat/Weapons/DA_Weapon_HandSaw.DA_Weapon_HandSaw"),
		TEXT("/Game/ChopIt/Combat/Weapons/DA_Weapon_SawHalo.DA_Weapon_SawHalo")
	};
	for (const TCHAR* Path : Paths)
	{
		if (UChopItWeaponDefinition* Weapon = LoadObject<UChopItWeaponDefinition>(nullptr, Path)) { Catalog.Add(Weapon); }
	}
}

bool AChopItShopTerminal::CanInteract_Implementation(AActor* Interactor) const
{
	return IsValid(Interactor) && Catalog.Num() > 0;
}

bool AChopItShopTerminal::Interact_Implementation(AActor* Interactor)
{
	const APawn* Pawn = Cast<APawn>(Interactor);
	AChopItPlayerState* PlayerState = Pawn ? Pawn->GetPlayerState<AChopItPlayerState>() : nullptr;
	if (!PlayerState || !PlayerState->GetShopComponent()) { return false; }
	TArray<UChopItWeaponDefinition*> Offers;
	for (UChopItWeaponDefinition* Weapon : Catalog) { if (Weapon) { Offers.Add(Weapon); } }
	PlayerState->GetShopComponent()->OpenShop(Offers);
	return true;
}
