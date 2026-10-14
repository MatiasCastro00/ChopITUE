#include "UI/Phone/ChopItPhonePromptBinding.h"

#include "UI/Phone/ChopItPhonePromptConfig.h"
#include "UI/Phone/ChopItPhonePromptViewModel.h"
#include "UI/Phone/ChopItPhonePromptWidget.h"
#include "UI/ChopItHUD.h"
#include "Phone/ChopItMysteryPhone.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

bool UChopItPhonePromptBinding::CanShowFor(const AChopItMysteryPhone* InPhone, const APawn* InPawn)
{
	return IsValid(InPhone) && IsValid(InPawn)
		&& InPhone->CanInteract_Implementation(const_cast<APawn*>(InPawn));
}

void UChopItPhonePromptBinding::Initialize(APlayerController* InPlayer, AChopItMysteryPhone* InPhone,
	UChopItPhonePromptConfig* InConfig)
{
	Player = InPlayer;
	Phone = InPhone;
	Config = InConfig;
	if (!InPlayer || !InPhone || !InConfig || !InConfig->IsValidConfig() || !InPhone->GetPhoneState()) return;
	ViewModel = NewObject<UChopItPhonePromptViewModel>(this);
	ViewModel->SetActionText(FText::GetEmpty());
	ViewModel->SetKeyText(InConfig->FallbackKeyText);

	DetectionSphere = InPhone->GetPromptDetection();
	PromptWidget = InPhone->GetPromptWidget();
	if (!DetectionSphere || !PromptWidget) return;
	DetectionSphere->SetSphereRadius(InPhone->InteractionRange + InConfig->DetectionPadding);
	DetectionSphere->OnComponentBeginOverlap.AddDynamic(this, &UChopItPhonePromptBinding::HandleOverlapBegin);
	DetectionSphere->OnComponentEndOverlap.AddDynamic(this, &UChopItPhonePromptBinding::HandleOverlapEnd);
	PromptWidget->SetRelativeLocation(FVector(0, 0, InConfig->HeightAbovePhone));
	PromptWidget->SetOwnerPlayer(InPlayer->GetLocalPlayer());
	PromptWidget->SetDrawSize(InConfig->DrawSize);
	if (!PromptWidget->GetWidgetClass()) PromptWidget->SetWidgetClass(InConfig->WidgetClass);
	PromptWidget->SetVisibility(false);
	PromptWidget->InitWidget();
	if (UChopItPhonePromptWidget* Widget = Cast<UChopItPhonePromptWidget>(PromptWidget->GetUserWidgetObject()))
		Widget->SetViewModel(ViewModel);
	InPhone->GetPhoneState()->OnStateChanged.AddDynamic(this, &UChopItPhonePromptBinding::HandleStateChanged);
	if (UWorld* World = InPlayer->GetWorld())
		World->GetTimerManager().SetTimerForNextTick(this, &UChopItPhonePromptBinding::CheckNearbyPlayer);
}

void UChopItPhonePromptBinding::Shutdown()
{
	StopCheck();
	if (Phone.IsValid() && Phone->GetPhoneState())
		Phone->GetPhoneState()->OnStateChanged.RemoveDynamic(this, &UChopItPhonePromptBinding::HandleStateChanged);
	if (DetectionSphere)
	{
		DetectionSphere->OnComponentBeginOverlap.RemoveDynamic(this, &UChopItPhonePromptBinding::HandleOverlapBegin);
		DetectionSphere->OnComponentEndOverlap.RemoveDynamic(this, &UChopItPhonePromptBinding::HandleOverlapEnd);
		DetectionSphere = nullptr;
	}
	if (PromptWidget)
	{
		if (UChopItPhonePromptWidget* Widget = Cast<UChopItPhonePromptWidget>(PromptWidget->GetUserWidgetObject()))
			Widget->SetViewModel(nullptr);
		PromptWidget->SetVisibility(false);
		PromptWidget = nullptr;
	}
	ViewModel = nullptr;
}

void UChopItPhonePromptBinding::HandleOverlapBegin(UPrimitiveComponent* Overlapped, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (Player.IsValid() && OtherActor == Player->GetPawn()) CheckNearbyPlayer();
}

void UChopItPhonePromptBinding::HandleOverlapEnd(UPrimitiveComponent* Overlapped, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (Player.IsValid() && OtherActor == Player->GetPawn()) StopCheck();
}

void UChopItPhonePromptBinding::HandleStateChanged(EChopItPhoneState Previous, EChopItPhoneState Current)
{
	if (Current == EChopItPhoneState::Answered || Current == EChopItPhoneState::Transforming ||
		Current == EChopItPhoneState::Dialogue || Current == EChopItPhoneState::Completed)
		StopCheck();
	else CheckNearbyPlayer();
}

void UChopItPhonePromptBinding::CheckNearbyPlayer()
{
	if (!Player.IsValid() || !Phone.IsValid() || !DetectionSphere) { StopCheck(); return; }
	APawn* Pawn = Player->GetPawn();
	if (!Pawn || !DetectionSphere->IsOverlappingActor(Pawn)) { StopCheck(); return; }
	if (UWorld* World = Player->GetWorld())
		if (!World->GetTimerManager().IsTimerActive(CheckTimer))
			World->GetTimerManager().SetTimer(CheckTimer, this, &UChopItPhonePromptBinding::RefreshPrompt,
				Config->CheckPeriod, true);
	RefreshPrompt();
}

void UChopItPhonePromptBinding::RefreshPrompt()
{
	if (!Player.IsValid() || !Phone.IsValid() || !DetectionSphere) { StopCheck(); return; }
	APawn* Pawn = Player->GetPawn();
	if (!Pawn || !DetectionSphere->IsOverlappingActor(Pawn)) { StopCheck(); return; }
	const AChopItHUD* HUD = Player->GetHUD<AChopItHUD>();
	const bool bShow = CanShowFor(Phone.Get(), Pawn) && !(HUD && HUD->IsModalOverlayActive());
	if (ViewModel) ViewModel->SetVisible(bShow);
	if (PromptWidget) PromptWidget->SetVisibility(bShow);
	if (Phone->GetPhoneState() && Phone->GetPhoneState()->GetState() >= EChopItPhoneState::Answered)
		StopCheck();
}

void UChopItPhonePromptBinding::HidePrompt()
{
	if (ViewModel) ViewModel->SetVisible(false);
	if (PromptWidget) PromptWidget->SetVisibility(false);
}

void UChopItPhonePromptBinding::StopCheck()
{
	if (Player.IsValid() && Player->GetWorld()) Player->GetWorld()->GetTimerManager().ClearTimer(CheckTimer);
	HidePrompt();
}
