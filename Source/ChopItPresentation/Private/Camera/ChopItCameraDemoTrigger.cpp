#include "Camera/ChopItCameraDemoTrigger.h"
#include "Camera/ChopItCameraAnchor.h"
#include "Camera/ChopItCameraCue.h"
#include "Camera/ChopItCameraDirectorSubsystem.h"
#include "Components/StaticMeshComponent.h"
#include "Core/CameraShakeAsset.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

AChopItCameraDemoTrigger::AChopItCameraDemoTrigger()
{
	PrimaryActorTick.bCanEverTick = false;
	Visual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Visual"));
	SetRootComponent(Visual);
	Visual->SetCollisionObjectType(ECC_WorldDynamic);
	Visual->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Visual->SetRelativeScale3D(FVector(0.55f, 0.55f, 1.2f));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Mesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (Mesh.Succeeded()) Visual->SetStaticMesh(Mesh.Object);
}

UChopItCameraDirectorSubsystem* AChopItCameraDemoTrigger::GetDirector() const
{
	APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	return PC && PC->GetLocalPlayer() ? PC->GetLocalPlayer()->GetSubsystem<UChopItCameraDirectorSubsystem>() : nullptr;
}

bool AChopItCameraDemoTrigger::CanInteract_Implementation(AActor* Interactor) const { return IsValid(Interactor) && !CueHandle.IsValid(); }

bool AChopItCameraDemoTrigger::Interact_Implementation(AActor* Interactor)
{
	UChopItCameraDirectorSubsystem* Director = GetDirector();
	if (!Director || !Interactor || !GetWorld()) return false;
	FActorSpawnParameters Params; Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	const FVector CameraLocation = Interactor->GetActorLocation() + FVector(-320.0f, 260.0f, 210.0f);
	RuntimeAnchor = GetWorld()->SpawnActor<AChopItCameraAnchor>(CameraLocation, (Interactor->GetActorLocation() + FVector(0,0,80) - CameraLocation).Rotation(), Params);
	const UChopItCameraCue* Cue = LoadObject<UChopItCameraCue>(nullptr, TEXT("/Game/ChopIt/Presentation/Camera/Cues/CC_DialogueCloseup.CC_DialogueCloseup"));
	const UChopItCameraEffectPreset* Effect = LoadObject<UChopItCameraEffectPreset>(nullptr, TEXT("/Game/ChopIt/Presentation/Camera/Effects/CE_DialogueFocus.CE_DialogueFocus"));
	CueHandle = Director->PushCameraCue(Cue, RuntimeAnchor, Interactor);
	EffectHandle = Director->PushCameraEffect(Effect, -1.0f);
	GetWorld()->GetTimerManager().SetTimer(ShakeTimer, this, &AChopItCameraDemoTrigger::PlayShakeStep, 0.8f, false);
	GetWorld()->GetTimerManager().SetTimer(EffectTimer, this, &AChopItCameraDemoTrigger::RemoveEffectStep, 1.55f, false);
	GetWorld()->GetTimerManager().SetTimer(RestoreTimer, this, &AChopItCameraDemoTrigger::RestoreStep, 2.25f, false);
	return CueHandle.IsValid();
}

void AChopItCameraDemoTrigger::PlayShakeStep()
{
	if (UChopItCameraDirectorSubsystem* Director = GetDirector()) if (const UCameraShakeAsset* Shake = LoadObject<UCameraShakeAsset>(nullptr, TEXT("/Game/ChopIt/Presentation/Camera/Shakes/CS_Critical.CS_Critical"))) Director->PlayCameraShake(Shake, 1.0f, GetActorLocation());
}
void AChopItCameraDemoTrigger::RemoveEffectStep() { if (UChopItCameraDirectorSubsystem* Director = GetDirector()) Director->PopCameraEffect(EffectHandle); EffectHandle.Invalidate(); }
void AChopItCameraDemoTrigger::RestoreStep() { if (UChopItCameraDirectorSubsystem* Director = GetDirector()) Director->PopCameraCue(CueHandle); CueHandle.Invalidate(); if (RuntimeAnchor) RuntimeAnchor->Destroy(); RuntimeAnchor = nullptr; }

void AChopItCameraDemoTrigger::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (GetWorld()) { GetWorld()->GetTimerManager().ClearTimer(ShakeTimer); GetWorld()->GetTimerManager().ClearTimer(EffectTimer); GetWorld()->GetTimerManager().ClearTimer(RestoreTimer); }
	if (UChopItCameraDirectorSubsystem* Director = GetDirector()) { Director->StopCameraRequest(CueHandle); Director->StopCameraRequest(EffectHandle); }
	Super::EndPlay(EndPlayReason);
}
