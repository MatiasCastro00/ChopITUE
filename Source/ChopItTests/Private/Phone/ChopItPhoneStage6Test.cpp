#include "Phone/ChopItMysteryPhone.h"
#include "Phone/ChopItPhoneVariantData.h"
#include "UI/Phone/ChopItPhoneDialogueScene.h"
#include "UI/Phone/ChopItPhoneConversationWidget.h"
#include "UI/Phone/ChopItPhonePresentationWidget.h"
#include "UI/Phone/ChopItPhonePresentationViewModel.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/Button.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Phone/ChopItPhoneBargainDefinition.h"
#include "Progression/ChopItUpgradeOfferComponent.h"
#include "Combat/ChopItCombatStatsComponent.h"
#include "Combat/ChopItHealthComponent.h"
#include "Dialogue/ChopItDialogueAssets.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChopItPhoneStage6Test, "ChopIt.Phone.Stage6.DialoguePresentation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FChopItPhoneStage6Test::RunTest(const FString& Parameters)
{
	for (const TCHAR* Id : {TEXT("Devil"), TEXT("Angel")})
	{
		const FString VariantPath = FString::Printf(TEXT("/Game/ChopIt/Phone/DA_Phone%s.DA_Phone%s"), Id, Id);
		UChopItPhoneVariantData* Variant = LoadObject<UChopItPhoneVariantData>(nullptr, *VariantPath);
		if (!TestNotNull(TEXT("Variant asset"), Variant)) return false;
		UChopItDialogueSequence* Sequence = Variant->DialogueSequence.LoadSynchronous();
		if (!TestNotNull(TEXT("Variant conversation"), Sequence)) return false;
		TArray<FText> Errors;
		TestTrue(TEXT("Editable conversation is valid"), Sequence->ValidateSequence(Errors));
		TestTrue(TEXT("Dialogue pauses gameplay"), Sequence->bPauseWorld && Sequence->bBlockGameplayInput);
		TestTrue(TEXT("Dialogue can be cancelled"), Sequence->bCanCancel);
		const FChopItDialogueLine* Offer = Sequence->FindLine(TEXT("Offer"));
		if (!TestNotNull(TEXT("Offer line"), Offer)) return false;
		TestNotNull(TEXT("Authored phone dialogue class"), Sequence->WidgetClass.Get());
        TestEqual(TEXT("Three bargains in authored fallback"), Offer->Choices.Num(), 3);
        for (const FChopItDialogueChoice& Choice : Offer->Choices)
		{
			TestTrue(TEXT("Bargain closes the offer"), Choice.NextLineId.IsNone());
			TestTrue(TEXT("Bargain shows its price"), Choice.Description.ToString().Contains(TEXT("PRECIO:")));
			bool bFoundBargain = false;
			for (const TCHAR* BargainName : {TEXT("HachaDeVidrio"), TEXT("SierraFrenetica"), TEXT("BrazoInfinito")})
			{
				const FString Path = FString::Printf(TEXT("/Game/ChopIt/Phone/Bargains/DA_PhoneBargain_%s.DA_PhoneBargain_%s"), BargainName, BargainName);
				if (const UChopItPhoneBargainDefinition* Bargain = LoadObject<UChopItPhoneBargainDefinition>(nullptr, *Path))
					bFoundBargain |= Bargain->BargainId == Choice.ChoiceId && !Bargain->Modifiers.IsEmpty();
			}
			TestTrue(TEXT("Each choice maps to a real bargain"), bFoundBargain);
		}
		for (const FChopItDialogueLine& Line : Sequence->Lines)
		{
			TestFalse(TEXT("Placeholder line does not grant rewards"), Line.StartEvent.IsValid() || Line.EndEvent.IsValid());
			for (const FChopItDialogueChoice& Choice : Line.Choices)
				TestFalse(TEXT("Placeholder choice does not grant rewards"), Choice.EventTag.IsValid());
		}
	}

	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	ON_SCOPE_EXIT { World->DestroyWorld(false); GEngine->DestroyWorldContext(World); };
	UChopItUpgradeOfferComponent* PhoneProgression = NewObject<UChopItUpgradeOfferComponent>(World);
	UChopItCombatStatsComponent* PhoneStats = NewObject<UChopItCombatStatsComponent>(World);
	UChopItHealthComponent* PhoneHealth = NewObject<UChopItHealthComponent>(World);
	UChopItPhoneBargainDefinition* PhoneReward = LoadObject<UChopItPhoneBargainDefinition>(nullptr,
		TEXT("/Game/ChopIt/Phone/Bargains/DA_PhoneBargain_HachaDeVidrio.DA_PhoneBargain_HachaDeVidrio"));
	if (!TestNotNull(TEXT("Phone bargain definition"), PhoneReward)) return false;
	const int32 BeforeModifiers = PhoneStats->GetModifierCount();
	TestTrue(TEXT("Selecting phone bargain applies both sides"), PhoneProgression->ApplyPhoneBargain(PhoneReward, PhoneStats, PhoneHealth));
	TestEqual(TEXT("Phone bargain changes combat stats"), PhoneStats->GetModifierCount(), BeforeModifiers + PhoneReward->Modifiers.Num());
	TestEqual(TEXT("Phone bargain records a stack"), PhoneProgression->GetPhoneBargainStackCount(PhoneReward->BargainId), 1);
	TestTrue(TEXT("Phone bargain doubles damage"), PhoneStats->EvaluateStat(EChopItCombatStat::Damage, 10.f) > 20.f);
	TestTrue(TEXT("Phone bargain lowers maximum health"), PhoneHealth->GetMaxHealth() < 100.f);
	TestTrue(TEXT("Health cost preserves current health percentage"), FMath::IsNearlyEqual(PhoneHealth->GetCurrentHealth(), PhoneHealth->GetMaxHealth()));
	for (const TCHAR* BargainName : {TEXT("SierraFrenetica"), TEXT("BrazoInfinito"), TEXT("FiloHambriento"),
		TEXT("TroncosMalditos"), TEXT("PiesPrestados"), TEXT("VerdugoCercano"), TEXT("FortunaDeSangre"),
		TEXT("UltimoAliento"), TEXT("GuardiaInmovil")})
	{
		const FString Path = FString::Printf(TEXT("/Game/ChopIt/Phone/Bargains/DA_PhoneBargain_%s.DA_PhoneBargain_%s"), BargainName, BargainName);
		const UChopItPhoneBargainDefinition* Bargain = LoadObject<UChopItPhoneBargainDefinition>(nullptr, *Path);
		if (!TestNotNull(TEXT("Editable bargain in offer catalog"), Bargain)) return false;
		TestTrue(TEXT("Bargain declares benefit and price"), !Bargain->BenefitText.IsEmpty() && !Bargain->CostText.IsEmpty());
	}
	UClass* ViewClass = LoadClass<UChopItPhoneConversationWidget>(nullptr,
		TEXT("/Game/ChopIt/Phone/WBP_PhoneConversation.WBP_PhoneConversation_C"));
	if (!TestNotNull(TEXT("Saved conversation WBP"), ViewClass)) return false;
	UChopItPhoneConversationWidget* View = CreateWidget<UChopItPhoneConversationWidget>(World, ViewClass);
	if (!TestNotNull(TEXT("Authored view can instantiate"), View)) return false;
	View->TakeWidget();
	View->ConfigureTheme(nullptr, true, 1);
	FChopItDialogueLine PreviewLine;
	FChopItDialogueMarkupDocument PreviewDocument;
	View->ShowLine(PreviewLine, PreviewDocument);
	TArray<FChopItDialogueChoiceView> Responses;
	Responses.SetNum(3);
	for (int32 Index = 0; Index < 3; ++Index) Responses[Index].Text = FText::AsNumber(Index);
	View->SetChoices(Responses, 1);
	int32 HoveredChoice = INDEX_NONE;
	View->OnChoiceHovered.BindLambda([&HoveredChoice, View](int32 Index)
	{
		HoveredChoice = Index;
		View->SetSelectedChoice(Index);
	});
	if (UButton* ThirdButton = Cast<UButton>(View->WidgetTree->FindWidget(TEXT("ChoiceButton3"))))
	{
		ThirdButton->OnHovered.Broadcast();
		TestEqual(TEXT("Mouse hover selects third phone offer"), HoveredChoice, 2);
		if (UBorder* ThirdCard = Cast<UBorder>(View->WidgetTree->FindWidget(TEXT("Card3"))))
			TestTrue(TEXT("Hovered phone offer gets visible highlight"), ThirdCard->GetBrushColor().R > .9f);
	}
	for (const TCHAR* Name : {TEXT("ChoiceButton1"), TEXT("ChoiceButton2"), TEXT("ChoiceButton3"), TEXT("RefreshButton"), TEXT("SkipButton")})
		TestNotNull(TEXT("Clickable authored offer control"), View->WidgetTree->FindWidget(Name));
	for (const TCHAR* Name : {TEXT("CardCost1"), TEXT("CardCost2"), TEXT("CardCost3")})
	{
		UTextBlock* Price = Cast<UTextBlock>(View->WidgetTree->FindWidget(Name));
		if (TestNotNull(TEXT("Visible price row on every card"), Price))
		{
			TestTrue(TEXT("Price font fits three lines"), Price->GetFont().Size <= 14);
		}
	}
	TestNull(TEXT("Caller identity is conveyed without a name label"), View->WidgetTree->FindWidget(TEXT("CallerName")));
	UHorizontalBox* Cards = Cast<UHorizontalBox>(View->WidgetTree->FindWidget(TEXT("ResponseCards")));
	if (Cards)
	{
		const UCanvasPanelSlot* CardSlot = Cast<UCanvasPanelSlot>(Cards->Slot);
		if (TestNotNull(TEXT("Offer cards use authored canvas slot"), CardSlot))
			TestTrue(TEXT("Cards end above action buttons"), CardSlot->GetPosition().Y + CardSlot->GetSize().Y <= 604.f);
	}
	View->ShowLine(PreviewLine, PreviewDocument);
	if (TestNotNull(TEXT("Authored response container"), Cards))
		TestEqual(TEXT("New line hides previous responses"), Cards->GetVisibility(), ESlateVisibility::Collapsed);
	UClass* PresentationClass = LoadClass<UChopItPhonePresentationWidget>(nullptr,
		TEXT("/Game/ChopIt/Phone/WBP_PhonePresentation.WBP_PhonePresentation_C"));
	if (!TestNotNull(TEXT("Saved presentation WBP"), PresentationClass)) return false;
	UChopItPhonePresentationWidget* Presentation = CreateWidget<UChopItPhonePresentationWidget>(World, PresentationClass);
	if (!TestNotNull(TEXT("Presentation instantiates"), Presentation)) return false;
	Presentation->TakeWidget();
	TestNotNull(TEXT("Full screen dimmer authored"), Presentation->WidgetTree->FindWidget(TEXT("WorldDimmer")));
	TestNotNull(TEXT("Phone portrait authored"), Presentation->WidgetTree->FindWidget(TEXT("Portrait")));
	TestNull(TEXT("No caller nameplate"), Presentation->WidgetTree->FindWidget(TEXT("NamePlate")));
	AChopItMysteryPhone* Phone = World->SpawnActor<AChopItMysteryPhone>(FVector::ZeroVector, FRotator::ZeroRotator);
	AChopItPhoneDialogueScene* Scene = World->SpawnActor<AChopItPhoneDialogueScene>(FVector(0, 0, 100000), FRotator::ZeroRotator);
	if (!TestNotNull(TEXT("Phone"), Phone) || !TestNotNull(TEXT("Capture stage"), Scene)) return false;
	UChopItPhoneVariantData* Devil = LoadObject<UChopItPhoneVariantData>(nullptr,
		TEXT("/Game/ChopIt/Phone/DA_PhoneDevil.DA_PhoneDevil"));
	TestTrue(TEXT("3D stage can clone the selected phone with its UMG composite"),
		Scene->InitializeScene(Phone, Devil) && Scene->GetTexture() && Scene->GetCompositeMaterial());
	return true;
}
#endif
