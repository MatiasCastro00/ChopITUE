#include "UI/Phone/ChopItPhonePromptBinding.h"
#include "UI/Phone/ChopItPhonePromptConfig.h"
#include "UI/Phone/ChopItPhonePromptViewModel.h"
#include "UI/Phone/ChopItPhonePromptWidget.h"
#include "Phone/ChopItMysteryPhone.h"
#include "Phone/ChopItPhoneStateComponent.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/DefaultPawn.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChopItPhoneStage4Test, "ChopIt.Phone.Stage4.PromptEligibility",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FChopItPhoneStage4Test::RunTest(const FString& Parameters)
{
	const UChopItPhonePromptConfig* Config = LoadObject<UChopItPhonePromptConfig>(nullptr,
		TEXT("/Game/ChopIt/Phone/DA_PhonePrompt.DA_PhonePrompt"));
	if (!TestNotNull(TEXT("Prompt Data Asset"), Config)) return false;
	TestTrue(TEXT("Prompt config valid"), Config->IsValidConfig());
	TestNotNull(TEXT("Editable prompt widget Blueprint"), Config->WidgetClass.Get());
	const UWidgetBlueprintGeneratedClass* WidgetClass = Cast<UWidgetBlueprintGeneratedClass>(Config->WidgetClass.Get());
	if (!TestNotNull(TEXT("Authored UMG widget class"), WidgetClass) ||
		!TestNotNull(TEXT("Authored UMG Designer tree"), WidgetClass->GetWidgetTreeArchetype())) return false;
	for (const TCHAR* Name : { TEXT("KeyPlate"), TEXT("KeyGlyph"), TEXT("RayLeft"),
		TEXT("RayCenter"), TEXT("RayRight") })
	{
		const UImage* Image = Cast<UImage>(WidgetClass->GetWidgetTreeArchetype()->FindWidget(Name));
		if (!TestNotNull(*FString::Printf(TEXT("Authored prompt sprite %s"), Name), Image)) return false;
		TestNotNull(*FString::Printf(TEXT("Prompt sprite texture %s"), Name), Image->GetBrush().GetResourceObject());
	}
	const UClass* PhoneClass = LoadClass<AChopItMysteryPhone>(nullptr,
		TEXT("/Game/ChopIt/Phone/BP_MysteryPhone.BP_MysteryPhone_C"));
	if (!TestNotNull(TEXT("Authored phone Blueprint"), PhoneClass)) return false;
	const AChopItMysteryPhone* PhoneDefaults = PhoneClass->GetDefaultObject<AChopItMysteryPhone>();
	TestTrue(TEXT("Phone Blueprint references the authored UMG prompt"),
		PhoneDefaults->GetPromptWidget() && PhoneDefaults->GetPromptWidget()->GetWidgetClass() == Config->WidgetClass.Get());

	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	ON_SCOPE_EXIT { World->DestroyWorld(false); GEngine->DestroyWorldContext(World); };
	AChopItMysteryPhone* Phone = World->SpawnActor<AChopItMysteryPhone>(FVector::ZeroVector, FRotator::ZeroRotator);
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	APawn* Pawn = World->SpawnActor<ADefaultPawn>(ADefaultPawn::StaticClass(), FVector(100, 0, 0),
		FRotator::ZeroRotator, Params);
	if (!TestNotNull(TEXT("Phone"), Phone) || !TestNotNull(TEXT("Player"), Pawn)) return false;
	TestTrue(TEXT("Prompt detection and widget are authored actor components"),
		Phone->GetPromptDetection() && Phone->GetPromptWidget() &&
		Phone->GetPromptDetection()->CreationMethod == EComponentCreationMethod::Native &&
		Phone->GetPromptWidget()->CreationMethod == EComponentCreationMethod::Native);
	TestTrue(TEXT("Nearby available phone may show prompt"), UChopItPhonePromptBinding::CanShowFor(Phone, Pawn));
	Pawn->SetActorLocation(FVector(Phone->InteractionRange + 20, 0, 0));
	TestFalse(TEXT("Outside gameplay range hides prompt"), UChopItPhonePromptBinding::CanShowFor(Phone, Pawn));
	Pawn->SetActorLocation(FVector(100, 0, 0));
	TestTrue(TEXT("Ring transition"), Phone->GetPhoneState()->TryTransition(EChopItPhoneState::Ringing));
	TestTrue(TEXT("Ringing phone remains interactable"), UChopItPhonePromptBinding::CanShowFor(Phone, Pawn));
	TestTrue(TEXT("Answer accepted once"), Phone->Interact_Implementation(Pawn));
	TestFalse(TEXT("Answered phone hides prompt"), UChopItPhonePromptBinding::CanShowFor(Phone, Pawn));
	TestFalse(TEXT("Duplicate answer rejected"), Phone->Interact_Implementation(Pawn));

	UChopItPhonePromptViewModel* VM = NewObject<UChopItPhonePromptViewModel>();
	int32 VisibilityEvents = 0;
	VM->AddFieldValueChangedDelegate(UChopItPhonePromptViewModel::FFieldNotificationClassDescriptor::bVisible,
		UChopItPhonePromptViewModel::FFieldValueChangedDelegate::CreateLambda(
			[&VisibilityEvents](UObject*, UE::FieldNotification::FFieldId) { ++VisibilityEvents; }));
	VM->SetVisible(true);
	VM->SetVisible(true);
	VM->SetVisible(false);
	TestEqual(TEXT("Prompt MVVM notifies visibility changes"), VisibilityEvents, 2);
	return true;
}
#endif
