#include "Dialogue/ChopItDialogueStageCharacter.h"

#include "Components/BillboardComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/ChopItCameraFacingTextComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInterface.h"

AChopItDialogueStageCharacter::AChopItDialogueStageCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	PrimaryActorTick.bTickEvenWhenPaused = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	Portrait = CreateDefaultSubobject<UBillboardComponent>(TEXT("Portrait"));
	Portrait->SetupAttachment(SceneRoot);
	Portrait->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Portrait->SetGenerateOverlapEvents(false);
	Portrait->bIsScreenSizeScaled = false;
	Portrait->ScreenSize = 0.0035f;

	auto CreatePillPart = [this](const FName Name)
	{
		UStaticMeshComponent* Part = CreateDefaultSubobject<UStaticMeshComponent>(Name);
		Part->SetupAttachment(SceneRoot);
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Part->SetGenerateOverlapEvents(false);
		Part->SetVisibility(false);
		return Part;
	};
	PillBody = CreatePillPart(TEXT("PillBody"));
	PillTop = CreatePillPart(TEXT("PillTop"));
	PillBottom = CreatePillPart(TEXT("PillBottom"));

	NameLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("NameLabel"));
	NameLabel->SetupAttachment(SceneRoot);
	NameLabel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	NameLabel->SetGenerateOverlapEvents(false);
	NameLabel->SetHorizontalAlignment(EHTA_Center);
	NameLabel->SetVerticalAlignment(EVRTA_TextCenter);
	NameLabel->SetWorldSize(38.0f);
	NameLabel->SetTextRenderColor(FColor(255, 169, 64));
	NameLabel->SetRelativeLocation(FVector(0.0f, 0.0f, 282.0f));
	NameLabel->SetVisibility(false);
	UChopItCameraFacingTextComponent::ConfigurePsxBypass(*NameLabel);
}

void AChopItDialogueStageCharacter::BeginPlay()
{
	Super::BeginPlay();
	// Placed stage characters load their sprite and relative scale from the map;
	// reconstruct the transient animation baseline before the first idle tick.
	RestLocation = GetActorLocation();
	bUsePillMarker = PillBody && PillBody->IsVisible() && Portrait && !Portrait->IsVisible();
	BaseScale = bUsePillMarker ? 1.0f : FMath::Max(0.05f, Portrait ? Portrait->GetRelativeScale3D().X : 1.0f);
}

void AChopItDialogueStageCharacter::Configure(UTexture2D* PortraitTexture, const float Height)
{
	bUsePillMarker = false;
	SetActorScale3D(FVector::OneVector);
	Portrait->SetVisibility(true);
	if (PillBody) PillBody->SetVisibility(false);
	if (PillTop) PillTop->SetVisibility(false);
	if (PillBottom) PillBottom->SetVisibility(false);
	if (NameLabel) NameLabel->SetVisibility(false);
	Portrait->SetSprite(PortraitTexture);
	const float TextureHeight = PortraitTexture ? FMath::Max(1, PortraitTexture->GetSizeY()) : 1024;
	BaseScale = FMath::Max(0.05f, Height / TextureHeight);
	Portrait->SetRelativeScale3D(FVector(BaseScale));
	RestLocation = GetActorLocation();
}

void AChopItDialogueStageCharacter::ConfigurePillMarker(const FText& DisplayName)
{
	bUsePillMarker = true;
	BaseScale = 1.0f;
	SetActorScale3D(FVector::OneVector);
	RestLocation = GetActorLocation();
	Portrait->SetSprite(nullptr);
	Portrait->SetVisibility(false);

	UStaticMesh* Cylinder = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	UStaticMesh* Sphere = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	UMaterialInterface* BodyMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/ChopIt/World/Blockout/Materials/MI_Stone.MI_Stone"));
	UMaterialInterface* AccentMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/ChopIt/World/Blockout/Materials/MI_Roof.MI_Roof"));

	if (PillBody)
	{
		PillBody->SetStaticMesh(Cylinder);
		PillBody->SetMaterial(0, BodyMaterial);
		PillBody->SetRelativeLocation(FVector(0.0f, 0.0f, 120.0f));
		PillBody->SetRelativeScale3D(FVector(0.72f, 0.72f, 1.40f));
		PillBody->SetVisibility(true);
	}
	for (const TPair<UStaticMeshComponent*, float> Cap : {
		TPair<UStaticMeshComponent*, float>(PillBottom, 50.0f),
		TPair<UStaticMeshComponent*, float>(PillTop, 190.0f)})
	{
		if (!Cap.Key) continue;
		Cap.Key->SetStaticMesh(Sphere);
		Cap.Key->SetMaterial(0, AccentMaterial);
		Cap.Key->SetRelativeLocation(FVector(0.0f, 0.0f, Cap.Value));
		Cap.Key->SetRelativeScale3D(FVector(0.72f));
		Cap.Key->SetVisibility(true);
	}
	if (NameLabel)
	{
		NameLabel->SetText(DisplayName);
		NameLabel->SetVisibility(true);
	}
}

void AChopItDialogueStageCharacter::PlayReaction(const FName ReactionId)
{
	ActiveReaction = ReactionId;
	ReactionTime = 0.55f;
}

void AChopItDialogueStageCharacter::BeginExit()
{
	if (bExiting) return;
	bExiting = true;
	ExitTime = 0.0f;
	RestLocation = GetActorLocation();
}

void AChopItDialogueStageCharacter::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	IdleTime += DeltaSeconds;

	if (bExiting)
	{
		ExitTime += DeltaSeconds;
		const float Alpha = FMath::Clamp(ExitTime / 0.85f, 0.0f, 1.0f);
		SetActorLocation(RestLocation + FVector::UpVector * (160.0f * Alpha));
		if (bUsePillMarker) SetActorScale3D(FVector(FMath::Lerp(1.0f, 0.05f, Alpha)));
		else Portrait->SetRelativeScale3D(FVector(BaseScale * FMath::Lerp(1.0f, 0.05f, Alpha)));
		if (Alpha >= 1.0f) Destroy();
		return;
	}

	float Scale = BaseScale * (1.0f + FMath::Sin(IdleTime * 1.7f) * 0.018f);
	float Lift = FMath::Sin(IdleTime * 1.35f) * 4.0f;
	float Sway = FMath::Sin(IdleTime * 0.9f) * 1.5f;
	if (ReactionTime > 0.0f)
	{
		ReactionTime = FMath::Max(0.0f, ReactionTime - DeltaSeconds);
		const float Progress = 1.0f - ReactionTime / 0.55f;
		const float Envelope = FMath::Sin(Progress * PI);
		Scale *= 1.0f + 0.14f * Envelope;
		if (ActiveReaction == TEXT("Roar")) Sway += FMath::Sin(Progress * 15.0f * PI) * 8.0f * Envelope;
		else Lift += 28.0f * Envelope;
	}
	if (bUsePillMarker) SetActorScale3D(FVector(Scale));
	else Portrait->SetRelativeScale3D(FVector(Scale));
	SetActorLocation(RestLocation + FVector(Sway, 0.0f, Lift));
	if (bUsePillMarker && NameLabel)
	{
		if (const UWorld* World = GetWorld())
		{
			if (const APlayerController* PlayerController = World->GetFirstPlayerController())
			{
				if (const APlayerCameraManager* Camera = PlayerController->PlayerCameraManager)
				{
					NameLabel->SetWorldRotation((Camera->GetCameraLocation() - NameLabel->GetComponentLocation()).Rotation());
				}
			}
		}
	}
}
