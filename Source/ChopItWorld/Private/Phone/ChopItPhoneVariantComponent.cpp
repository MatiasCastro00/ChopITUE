#include "Phone/ChopItPhoneVariantComponent.h"
#include "Phone/ChopItPhoneVariantData.h"
#include "Phone/ChopItPhoneVariantPool.h"

UChopItPhoneVariantComponent::UChopItPhoneVariantComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	VariantPool = TSoftObjectPtr<UChopItPhoneVariantPool>(
		FSoftObjectPath(TEXT("/Game/NewChopIt/Art/Models/Phone/DA_PhoneVariants.DA_PhoneVariants")));
}

void UChopItPhoneVariantComponent::InitializeSelectionSeed(const int32 Seed)
{
	if (SelectedVariant) return;
	SelectionSeed = Seed;
	bHasSelectionSeed = true;
}

void UChopItPhoneVariantComponent::BeginPlay()
{
	Super::BeginPlay();
	PhoneState = GetOwner() ? GetOwner()->FindComponentByClass<UChopItPhoneStateComponent>() : nullptr;
	ActivePool = VariantPool.LoadSynchronous();
	if (!PhoneState || !ActivePool || !ActivePool->IsValidConfig())
	{
		UE_LOG(LogTemp, Warning, TEXT("Phone variant selection disabled on %s: missing state or valid variant pool"),
			*GetNameSafe(GetOwner()));
		return;
	}
	PhoneState->OnStateChanged.AddDynamic(this, &UChopItPhoneVariantComponent::HandleStateChanged);
	if (PhoneState->GetState() == EChopItPhoneState::Answered) SelectAfterAnswer();
}

void UChopItPhoneVariantComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (PhoneState) PhoneState->OnStateChanged.RemoveDynamic(this, &UChopItPhoneVariantComponent::HandleStateChanged);
	Super::EndPlay(EndPlayReason);
}

void UChopItPhoneVariantComponent::HandleStateChanged(EChopItPhoneState Previous, EChopItPhoneState Current)
{
	if (Current == EChopItPhoneState::Answered) SelectAfterAnswer();
}

void UChopItPhoneVariantComponent::SelectAfterAnswer()
{
	if (SelectedVariant || !PhoneState || PhoneState->GetState() != EChopItPhoneState::Answered ||
		!ActivePool || !ActivePool->IsValidConfig()) return;
	const int32 Seed = bHasSelectionSeed ? SelectionSeed : static_cast<int32>(GetTypeHash(GetOwner()->GetActorLocation()));
	FRandomStream Random(Seed);
	SelectedVariant = const_cast<UChopItPhoneVariantData*>(ActivePool->Select(Random));
	if (!SelectedVariant) return;
	OnVariantSelected.Broadcast(SelectedVariant);
	PhoneState->TryTransition(EChopItPhoneState::Transforming);
}
