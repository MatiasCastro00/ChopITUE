#include "Phone/ChopItPhoneRevealComponent.h"

#include "Phone/ChopItMysteryPhone.h"
#include "Phone/ChopItPhoneVariantComponent.h"
#include "Phone/ChopItPhoneVariantData.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Sound/SoundBase.h"

UChopItPhoneRevealComponent::UChopItPhoneRevealComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UChopItPhoneRevealComponent::BeginPlay()
{
	Super::BeginPlay();
	AActor* Owner = GetOwner();
	PhoneState = Owner ? Owner->FindComponentByClass<UChopItPhoneStateComponent>() : nullptr;
	VariantSelection = Owner ? Owner->FindComponentByClass<UChopItPhoneVariantComponent>() : nullptr;
	if (!Owner || !Owner->GetRootComponent() || !PhoneState || !VariantSelection) return;
	RevealLight = NewObject<UPointLightComponent>(Owner, TEXT("PhoneVariantLight"));
	Owner->AddInstanceComponent(RevealLight);
	RevealLight->SetupAttachment(Owner->GetRootComponent());
	RevealLight->SetRelativeLocation(FVector(0, 0, 55));
	RevealLight->SetCastShadows(false);
	RevealLight->SetIntensity(0.0f);
	RevealLight->SetVisibility(false);
	RevealLight->RegisterComponent();
	PhoneState->OnStateChanged.AddDynamic(this, &UChopItPhoneRevealComponent::HandleStateChanged);
	if (PhoneState->GetState() == EChopItPhoneState::Transforming) StartReveal();
}

void UChopItPhoneRevealComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (GetWorld()) GetWorld()->GetTimerManager().ClearTimer(RevealTimer);
	if (PhoneState) PhoneState->OnStateChanged.RemoveDynamic(this, &UChopItPhoneRevealComponent::HandleStateChanged);
	Super::EndPlay(EndPlayReason);
}

void UChopItPhoneRevealComponent::HandleStateChanged(EChopItPhoneState Previous, EChopItPhoneState Current)
{
	if (Current == EChopItPhoneState::Transforming) StartReveal();
	else if (Current == EChopItPhoneState::Completed && GetWorld())
		GetWorld()->GetTimerManager().ClearTimer(RevealTimer);
}

void UChopItPhoneRevealComponent::StartReveal()
{
	if (!PhoneState || !VariantSelection || !RevealLight || !GetWorld() ||
		PhoneState->GetState() != EChopItPhoneState::Transforming) return;
	const UChopItPhoneVariantData* Variant = VariantSelection->GetSelectedVariant();
	if (!Variant || !Variant->IsValidConfig()) return;
	if (const AChopItMysteryPhone* Phone = Cast<AChopItMysteryPhone>(GetOwner()))
	{
		if (UStaticMesh* Mesh = Variant->BaseMesh.LoadSynchronous()) Phone->GetBaseVisual()->SetStaticMesh(Mesh);
		if (UStaticMesh* Mesh = Variant->ReceiverMesh.LoadSynchronous()) Phone->GetReceiverVisual()->SetStaticMesh(Mesh);
		if (UMaterialInterface* Material = Variant->BaseMaterial.LoadSynchronous()) Phone->GetBaseVisual()->SetMaterial(0, Material);
		if (UMaterialInterface* Material = Variant->ReceiverMaterial.LoadSynchronous()) Phone->GetReceiverVisual()->SetMaterial(0, Material);
	}
	if (USoundBase* Sound = Variant->RevealSound.LoadSynchronous())
		UGameplayStatics::PlaySoundAtLocation(this, Sound, GetOwner()->GetActorLocation());
	if (UNiagaraSystem* VFX = Variant->RevealVFX.LoadSynchronous())
		UNiagaraFunctionLibrary::SpawnSystemAttached(VFX, GetOwner()->GetRootComponent(), NAME_None,
			FVector(0, 0, 55), FRotator::ZeroRotator, EAttachLocation::KeepRelativeOffset, true);
	RevealLight->SetLightColor(Variant->AccentColor);
	RevealLight->SetAttenuationRadius(Variant->LightRadius);
	RevealLight->SetIntensity(0.0f);
	RevealLight->SetVisibility(true);
	Elapsed = 0.0f;
	GetWorld()->GetTimerManager().ClearTimer(RevealTimer);
	GetWorld()->GetTimerManager().SetTimer(RevealTimer, this, &UChopItPhoneRevealComponent::UpdateReveal,
		FMath::Max(0.01f, UpdatePeriod), true);
}

void UChopItPhoneRevealComponent::UpdateReveal()
{
	if (!PhoneState || PhoneState->GetState() != EChopItPhoneState::Transforming ||
		!VariantSelection || !VariantSelection->GetSelectedVariant() || !RevealLight)
	{
		if (GetWorld()) GetWorld()->GetTimerManager().ClearTimer(RevealTimer);
		return;
	}
	const UChopItPhoneVariantData* Variant = VariantSelection->GetSelectedVariant();
	Elapsed += FMath::Max(0.01f, UpdatePeriod);
	const float Alpha = FMath::Clamp(Elapsed / Variant->RevealDuration, 0.0f, 1.0f);
	RevealLight->SetIntensity(FMath::Lerp(Variant->IdleLightIntensity, Variant->PeakLightIntensity,
		FMath::Sin(PI * Alpha)));
	if (Alpha >= 1.0f) FinishReveal();
}

void UChopItPhoneRevealComponent::CompleteRevealEarly()
{
	if (PhoneState && PhoneState->GetState() == EChopItPhoneState::Transforming) FinishReveal();
}

void UChopItPhoneRevealComponent::FinishReveal()
{
	if (!PhoneState || PhoneState->GetState() != EChopItPhoneState::Transforming || !VariantSelection) return;
	if (GetWorld()) GetWorld()->GetTimerManager().ClearTimer(RevealTimer);
	UChopItPhoneVariantData* Variant = VariantSelection->GetSelectedVariant();
	if (RevealLight && Variant) RevealLight->SetIntensity(Variant->IdleLightIntensity);
	if (PhoneState->TryTransition(EChopItPhoneState::Dialogue)) OnRevealFinished.Broadcast(Variant);
}
