#include "Items/ChopItScreamDebuffComponent.h"
#include "Combat/ChopItHealthComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

UChopItScreamDebuffComponent::UChopItScreamDebuffComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> FX(TEXT("/Game/ChopIt/Items/Mandrake/NS_Mandrake_Confusion.NS_Mandrake_Confusion"));
	ConfusionSystem = FX.Object;
}

void UChopItScreamDebuffComponent::Refresh(float Multiplier, float Duration)
{
	if (!GetOwner() || !GetWorld()) return;
	if (!Health.IsValid())
	{
		Health = GetOwner()->FindComponentByClass<UChopItHealthComponent>();
		if (Health.IsValid()) Health->OnDeath.AddUObject(this,&ThisClass::HandleDeath);
	}
	if (Health.IsValid() && !Health->IsAlive()) return;
	Multiplier = FMath::Clamp(Multiplier,.1f,1.f);
	if (!bActive)
	{
		Movement = GetOwner()->FindComponentByClass<UCharacterMovementComponent>();
		OriginalSpeed = Movement.IsValid() ? Movement->MaxWalkSpeed : 0.f;
		AppliedMultiplier = Multiplier;
		bActive = true;
		Confusion = NewObject<UNiagaraComponent>(GetOwner());
		GetOwner()->AddInstanceComponent(Confusion);
		Confusion->SetupAttachment(GetOwner()->GetRootComponent());
		const ACharacter* Character = Cast<ACharacter>(GetOwner());
		const float Height = Character ? Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()+24.f : 95.f;
		Confusion->SetRelativeLocation(FVector(0,0,Height));
		Confusion->SetAsset(ConfusionSystem);
		Confusion->RegisterComponent();
		Confusion->Activate(true);
	}
	else
	{
		// Respect an external change to movement tuning while the slow is active.
		if (Movement.IsValid() && !FMath::IsNearlyEqual(Movement->MaxWalkSpeed,AppliedSpeed)) OriginalSpeed = Movement->MaxWalkSpeed;
		AppliedMultiplier = FMath::Min(AppliedMultiplier,Multiplier);
	}
	AppliedSpeed = OriginalSpeed*AppliedMultiplier;
	if (Movement.IsValid()) Movement->MaxWalkSpeed = AppliedSpeed;
	SetComponentTickEnabled(true);
	GetWorld()->GetTimerManager().SetTimer(Expiry,this,&ThisClass::ClearSlow,FMath::Max(.05f,Duration),false);
}

void UChopItScreamDebuffComponent::TickComponent(float Dt,ELevelTick Type,FActorComponentTickFunction* Function)
{
	Super::TickComponent(Dt,Type,Function);
	Orbit += Dt*130.f;
	if (Confusion) Confusion->SetRelativeRotation(FRotator(0,Orbit,0));
}
void UChopItScreamDebuffComponent::ClearSlow()
{
	if (Movement.IsValid() && FMath::IsNearlyEqual(Movement->MaxWalkSpeed,AppliedSpeed)) Movement->MaxWalkSpeed = OriginalSpeed;
	bActive = false;
	SetComponentTickEnabled(false);
	if (Confusion) { Confusion->DestroyComponent(); Confusion = nullptr; }
}
void UChopItScreamDebuffComponent::HandleDeath(AActor*,AActor*)
{
	if (GetWorld()) GetWorld()->GetTimerManager().ClearTimer(Expiry);
	ClearSlow();
}
void UChopItScreamDebuffComponent::EndPlay(const EEndPlayReason::Type Reason)
{
	if (GetWorld()) GetWorld()->GetTimerManager().ClearTimer(Expiry);
	if (Health.IsValid()) Health->OnDeath.RemoveAll(this);
	ClearSlow();
	Super::EndPlay(Reason);
}
