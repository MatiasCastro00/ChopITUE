#include "Phone/ChopItPhoneWidgetEditorLibrary.h"

#include "WidgetBlueprint.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/ScaleBox.h"
#include "UI/ChopItAnimatedTextBlock.h"
#include "UI/Phone/ChopItPhoneConversationWidget.h"
#include "UI/Phone/ChopItPhonePresentationWidget.h"
#include "Styling/CoreStyle.h"
#include "Editor.h"
#include "UI/Phone/ChopItPhoneDialogueScene.h"
#include "UI/Phone/ChopItPhonePresentationViewModel.h"
#include "Phone/ChopItMysteryPhone.h"
#include "Phone/ChopItPhoneVariantData.h"
#include "Slate/WidgetRenderer.h"
#include "Widgets/SOverlay.h"
#include "ImageUtils.h"
#include "Misc/FileHelper.h"
#include "Serialization/BufferArchive.h"
#include "RenderingThread.h"
#include "AssetCompilingManager.h"
#include "ShaderCompiler.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Framework/Application/SlateApplication.h"

bool UChopItPhoneWidgetEditorLibrary::BuildEditablePhoneIndicator()
{
	UWidgetBlueprint* Blueprint = LoadObject<UWidgetBlueprint>(nullptr,
		TEXT("/Game/ChopIt/Phone/WBP_PhoneRingIndicator.WBP_PhoneRingIndicator"));
	if (!Blueprint || !Blueprint->WidgetTree) return false;
	if (Blueprint->WidgetTree->RootWidget) return true;

	UTexture2D* BubbleTexture = LoadObject<UTexture2D>(nullptr,
		TEXT("/Game/ChopIt/Phone/T_PhoneComicBubble.T_PhoneComicBubble"));
	UTexture2D* RingTexture = LoadObject<UTexture2D>(nullptr,
		TEXT("/Game/ChopIt/Phone/T_PhoneComicRing.T_PhoneComicRing"));
	if (!BubbleTexture || !RingTexture) return false;

	Blueprint->Modify();
	Blueprint->WidgetTree->Modify();
	UWidgetTree* Tree = Blueprint->WidgetTree;
	USizeBox* Frame = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("ComicFrame"));
	Frame->SetWidthOverride(240.0f);
	Frame->SetHeightOverride(150.0f);
	Tree->RootWidget = Frame;
	UCanvasPanel* Canvas = Tree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("ComicCanvas"));
	Frame->AddChild(Canvas);

	const auto AddImage = [Tree, Canvas](const TCHAR* Name, UTexture2D* Texture,
		FVector2D Position, FVector2D Size, int32 ZOrder, FLinearColor Color)
	{
		UImage* Image = Tree->ConstructWidget<UImage>(UImage::StaticClass(), FName(Name));
		Image->SetBrushFromTexture(Texture);
		Image->SetColorAndOpacity(Color);
		Image->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		UCanvasPanelSlot* Slot = Canvas->AddChildToCanvas(Image);
		Slot->SetPosition(Position);
		Slot->SetSize(Size);
		Slot->SetZOrder(ZOrder);
		return Image;
	};
	AddImage(TEXT("ComicBubble"), BubbleTexture, FVector2D(7.5f, 0), FVector2D(225, 150),
		0, FLinearColor(1.0f, 0.97f, 0.78f));
	AddImage(TEXT("RingLeft"), RingTexture, FVector2D(15, 10), FVector2D(118, 70),
		1, FLinearColor(1.0f, 0.80f, 0.28f));
	AddImage(TEXT("RingRight"), RingTexture, FVector2D(102, 68), FVector2D(108, 64),
		2, FLinearColor(1.0f, 0.80f, 0.28f));

	UTextBlock* Arrow = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DirectionArrow"));
	Arrow->SetText(FText::FromString(TEXT("➤")));
	FSlateFontInfo ArrowFont = Arrow->GetFont();
	ArrowFont.Size = 19;
	Arrow->SetFont(ArrowFont);
	Arrow->SetColorAndOpacity(FSlateColor(FLinearColor::Black));
	UCanvasPanelSlot* ArrowSlot = Canvas->AddChildToCanvas(Arrow);
	ArrowSlot->SetPosition(FVector2D(111, 128));
	ArrowSlot->SetSize(FVector2D(23, 21));
	ArrowSlot->SetZOrder(3);

	UTextBlock* Label = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("RingText"));
	FSlateFontInfo LabelFont = Label->GetFont();
	LabelFont.Size = 16;
	Label->SetFont(LabelFont);
	Label->SetColorAndOpacity(FSlateColor(FLinearColor::Black));
	UCanvasPanelSlot* LabelSlot = Canvas->AddChildToCanvas(Label);
	LabelSlot->SetPosition(FVector2D(57, 60));
	LabelSlot->SetSize(FVector2D(160, 30));
	LabelSlot->SetZOrder(3);

	FKismetEditorUtilities::CompileBlueprint(Blueprint);
	Blueprint->MarkPackageDirty();
	return true;
}

bool UChopItPhoneWidgetEditorLibrary::BuildEditablePhonePrompt()
{
	UWidgetBlueprint* Blueprint = LoadObject<UWidgetBlueprint>(nullptr,
		TEXT("/Game/ChopIt/Phone/WBP_PhoneInteractPrompt.WBP_PhoneInteractPrompt"));
	if (!Blueprint || !Blueprint->WidgetTree) return false;
	if (Blueprint->WidgetTree->RootWidget &&
		Blueprint->WidgetTree->FindWidget(TEXT("KeyGroup"))) return true;
	UTexture2D* KeyPlateTexture = LoadObject<UTexture2D>(nullptr,
		TEXT("/Game/ChopIt/Phone/T_PhonePromptKeyPlate.T_PhonePromptKeyPlate"));
	UTexture2D* KeyGlyphTexture = LoadObject<UTexture2D>(nullptr,
		TEXT("/Game/ChopIt/Phone/T_PhonePromptKeyE.T_PhonePromptKeyE"));
	UTexture2D* LeftTexture = LoadObject<UTexture2D>(nullptr,
		TEXT("/Game/ChopIt/Phone/T_PhonePromptRayLeft.T_PhonePromptRayLeft"));
	UTexture2D* CenterTexture = LoadObject<UTexture2D>(nullptr,
		TEXT("/Game/ChopIt/Phone/T_PhonePromptRayCenter.T_PhonePromptRayCenter"));
	UTexture2D* RightTexture = LoadObject<UTexture2D>(nullptr,
		TEXT("/Game/ChopIt/Phone/T_PhonePromptRayRight.T_PhonePromptRayRight"));
	if (!KeyPlateTexture || !KeyGlyphTexture || !LeftTexture || !CenterTexture || !RightTexture) return false;

	Blueprint->Modify();
	UWidgetTree* Tree = Blueprint->WidgetTree;
	Tree->Modify();
	USizeBox* Frame = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("PromptFrame"));
	Frame->SetWidthOverride(110.0f);
	Frame->SetHeightOverride(120.0f);
	Tree->RootWidget = Frame;
	UCanvasPanel* Canvas = Tree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("PromptCanvas"));
	Frame->AddChild(Canvas);
	const auto AddImage = [Tree](UCanvasPanel* Parent, const TCHAR* Name, UTexture2D* Texture,
		FVector2D Position, FVector2D Size, int32 ZOrder)
	{
		UImage* Image = Tree->ConstructWidget<UImage>(UImage::StaticClass(), FName(Name));
		Image->SetBrushFromTexture(Texture);
		Image->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		UCanvasPanelSlot* Slot = Parent->AddChildToCanvas(Image);
		Slot->SetPosition(Position);
		Slot->SetSize(Size);
		Slot->SetZOrder(ZOrder);
		return Image;
	};
	AddImage(Canvas, TEXT("RayLeft"), LeftTexture, FVector2D(25, 10), FVector2D(26, 29), 0);
	AddImage(Canvas, TEXT("RayCenter"), CenterTexture, FVector2D(44, 0), FVector2D(24, 34), 0);
	AddImage(Canvas, TEXT("RayRight"), RightTexture, FVector2D(63, 10), FVector2D(26, 29), 0);
	UCanvasPanel* KeyGroup = Tree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("KeyGroup"));
	UCanvasPanelSlot* GroupSlot = Canvas->AddChildToCanvas(KeyGroup);
	GroupSlot->SetPosition(FVector2D(0, 15));
	GroupSlot->SetSize(FVector2D(110, 105));
	GroupSlot->SetZOrder(1);
	KeyGroup->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	AddImage(KeyGroup, TEXT("KeyPlate"), KeyPlateTexture, FVector2D(2, 0), FVector2D(106, 106), 0);
	AddImage(KeyGroup, TEXT("KeyGlyph"), KeyGlyphTexture, FVector2D(18, 13), FVector2D(74, 74), 1);
	FKismetEditorUtilities::CompileBlueprint(Blueprint);
	Blueprint->MarkPackageDirty();
	return true;
}

bool UChopItPhoneWidgetEditorLibrary::BuildEditablePhoneDialogue()
{
    const auto MakeBlueprint = [](const TCHAR* Name, UClass* Parent)
    {
        const FString Path = FString(TEXT("/Game/ChopIt/Phone/")) + Name;
        UWidgetBlueprint* BP = LoadObject<UWidgetBlueprint>(nullptr, *(Path + TEXT(".") + Name));
        if (!BP)
        {
            UPackage* Package = CreatePackage(*Path);
            BP = Cast<UWidgetBlueprint>(FKismetEditorUtilities::CreateBlueprint(Parent, Package, FName(Name),
                BPTYPE_Normal, UWidgetBlueprint::StaticClass(), UWidgetBlueprintGeneratedClass::StaticClass()));
        }
        return BP;
    };
    const auto Place = [](UCanvasPanel* Canvas, UWidget* Child, FVector2D Position, FVector2D Size)
    {
        UCanvasPanelSlot* Slot = Canvas->AddChildToCanvas(Child);
        Slot->SetPosition(Position);
        Slot->SetSize(Size);
    };
    const auto Text = [](UWidgetTree* Tree, FName Name, const TCHAR* Value, int32 Size, FLinearColor Color)
    {
        UTextBlock* Label = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
        Label->SetText(FText::FromString(Value));
        FSlateFontInfo Font = Label->GetFont();
        Font.Size = Size;
        Font.TypefaceFontName = TEXT("Bold");
        Label->SetFont(Font);
        Label->SetColorAndOpacity(Color);
        Label->SetAutoWrapText(true);
        return Label;
    };
    const auto DesignCanvas = [](UWidgetTree* Tree)
    {
        UScaleBox* Scale = Tree->ConstructWidget<UScaleBox>(UScaleBox::StaticClass(), TEXT("ResponsiveScale"));
        Scale->SetStretch(EStretch::ScaleToFit);
        Tree->RootWidget = Scale;
        USizeBox* Size = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("DesignResolution"));
        Size->SetWidthOverride(1280);
        Size->SetHeightOverride(720);
        Scale->SetContent(Size);
        UCanvasPanel* Canvas = Tree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Layout"));
        Size->SetContent(Canvas);
        return Canvas;
    };
    UWidgetBlueprint* Presentation = MakeBlueprint(TEXT("WBP_PhonePresentation"), UChopItPhonePresentationWidget::StaticClass());
    UWidgetBlueprint* Conversation = MakeBlueprint(TEXT("WBP_PhoneConversation"), UChopItPhoneConversationWidget::StaticClass());
    if (!Presentation || !Conversation) return false;
    if (!Presentation->WidgetTree->RootWidget)
    {
        UWidgetTree* Tree = Presentation->WidgetTree;
        UCanvasPanel* Root = Tree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("PresentationRoot"));
        Tree->RootWidget = Root;
        UBorder* Dim = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("WorldDimmer"));
        Dim->SetBrushColor(FLinearColor(0.005f, .007f, .004f, .76f));
        UCanvasPanelSlot* DimSlot = Root->AddChildToCanvas(Dim);
        DimSlot->SetAnchors(FAnchors(0, 0, 1, 1));
        DimSlot->SetOffsets(FMargin(0));
        UCanvasPanel* Layout = DesignCanvas(Tree);
        UWidget* Responsive = Tree->RootWidget;
        Tree->RootWidget = Root;
        UCanvasPanelSlot* LayoutSlot = Root->AddChildToCanvas(Responsive);
        LayoutSlot->SetAnchors(FAnchors(0, 0, 1, 1));
        LayoutSlot->SetOffsets(FMargin(0));
        UImage* Portrait = Tree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Portrait"));
        Place(Layout, Portrait, FVector2D(-65, 95), FVector2D(540, 540));
        FKismetEditorUtilities::CompileBlueprint(Presentation);
        Presentation->MarkPackageDirty();
    }
    if (!Conversation->WidgetTree->RootWidget)
    {
        UWidgetTree* Tree = Conversation->WidgetTree;
        UCanvasPanel* Layout = DesignCanvas(Tree);
        const FLinearColor Orange(1, .36f, .035f);
        UBorder* Speech = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("SpeechPanel"));
        Speech->SetBrushColor(FLinearColor::Black);
        Speech->SetPadding(FMargin(28, 22));
        Place(Layout, Speech, FVector2D(380, 90), FVector2D(855, 210));
        UVerticalBox* Lines = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("DialogueLines"));
        Speech->SetContent(Lines);
        UChopItAnimatedTextBlock* Body = Tree->ConstructWidget<UChopItAnimatedTextBlock>(UChopItAnimatedTextBlock::StaticClass(), TEXT("BodyText"));
        Lines->AddChildToVerticalBox(Body);
        UHorizontalBox* Cards = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ResponseCards"));
        Place(Layout, Cards, FVector2D(460, 332), FVector2D(775, 272));
        for (int32 i = 1; i <= 3; ++i)
        {
            UBorder* Card = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), FName(*FString::Printf(TEXT("Card%d"), i)));
            Card->SetPadding(FMargin(3));
            Card->SetBrushColor(FLinearColor(.075f, .065f, .05f));
            UHorizontalBoxSlot* Slot = Cards->AddChildToHorizontalBox(Card);
            Slot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
            Slot->SetPadding(FMargin(0, 0, 12, 0));
            UBorder* Fill = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), FName(*FString::Printf(TEXT("CardFill%d"), i)));
            Fill->SetBrushColor(FLinearColor::Black);
            Fill->SetPadding(FMargin(20, 24));
            Card->SetContent(Fill);
            UVerticalBox* Content = Tree->ConstructWidget<UVerticalBox>();
            Fill->SetContent(Content);
            Content->AddChildToVerticalBox(Text(Tree, FName(*FString::Printf(TEXT("Response%d"), i)), TEXT("RESPUESTA"), 22, Orange));
            Content->AddChildToVerticalBox(Text(Tree, FName(*FString::Printf(TEXT("CardHint%d"), i)), TEXT("Selecciona para responder."), 17,
                FLinearColor(.88f, .87f, .81f)))->SetPadding(FMargin(0, 32, 0, 0));
        }
        Place(Layout, Text(Tree, TEXT("ContinueHint"), TEXT("E / ESPACIO  -  CONTINUAR"), 18, FLinearColor::White), FVector2D(410, 620), FVector2D(420, 32));
        Place(Layout, Text(Tree, TEXT("Controls"), TEXT("1-3  ELEGIR MEJORA     FLECHAS + ENTER  ELEGIR     R  RENOVAR     ESC  OMITIR"), 14,
            FLinearColor(.68f, .65f, .55f)), FVector2D(450, 674), FVector2D(790, 28));
        FKismetEditorUtilities::CompileBlueprint(Conversation);
        Conversation->MarkPackageDirty();
    }
    // Migrate existing editable WBP trees without replacing their authored layout.
    if (UWidget* NamePlate = Presentation->WidgetTree->FindWidget(TEXT("NamePlate"))) NamePlate->RemoveFromParent();
    if (UWidget* Caller = Conversation->WidgetTree->FindWidget(TEXT("CallerName"))) Caller->RemoveFromParent();
    if (UWidget* FourthCard = Conversation->WidgetTree->FindWidget(TEXT("Card4"))) FourthCard->RemoveFromParent();
    if (UTextBlock* Controls = Cast<UTextBlock>(Conversation->WidgetTree->FindWidget(TEXT("Controls"))))
        Controls->SetText(FText::FromString(TEXT("CLIC / 1-3 ELEGIR    MANDO: STICK + A    Y RENOVAR    B OMITIR")));
    UWidgetTree* ConversationTree = Conversation->WidgetTree;
    UCanvasPanel* Layout = Cast<UCanvasPanel>(ConversationTree->FindWidget(TEXT("Layout")));
    if (!Layout) return false;
    for (int32 i = 1; i <= 3; ++i)
    {
        const FString CostName = FString::Printf(TEXT("CardCost%d"), i);
        if (!ConversationTree->FindWidget(FName(*CostName)))
        {
            const FString FillName = FString::Printf(TEXT("CardFill%d"), i);
            UBorder* Fill = Cast<UBorder>(ConversationTree->FindWidget(FName(*FillName)));
            UVerticalBox* Content = Fill ? Cast<UVerticalBox>(Fill->GetContent()) : nullptr;
            if (!Content) return false;
            UTextBlock* Cost = Text(ConversationTree, FName(*CostName), TEXT("PRECIO"), 16,
                FLinearColor(1.0f, .43f, .27f));
            Cost->SetWrapTextAt(180);
            Content->AddChildToVerticalBox(Cost)->SetPadding(FMargin(0, 18, 0, 0));
        }
    }
    const auto AddButton = [ConversationTree, Layout, &Place, &Text](const TCHAR* Name, const TCHAR* Label,
        FVector2D Position, FVector2D Size, FLinearColor Background, FLinearColor Foreground, int32 FontSize)
    {
        UButton* Button = Cast<UButton>(ConversationTree->FindWidget(Name));
        if (!Button)
        {
            Button = ConversationTree->ConstructWidget<UButton>(UButton::StaticClass(), FName(Name));
            Place(Layout, Button, Position, Size);
        }
        Button->SetBackgroundColor(Background);
        if (!Button->GetContent()) Button->SetContent(Text(ConversationTree, FName(FString(Name) + TEXT("Label")), Label, FontSize, Foreground));
        return Button;
    };
    for (int32 i = 1; i <= 3; ++i)
    {
        AddButton(*FString::Printf(TEXT("ChoiceButton%d"), i), TEXT(""), FVector2D(460 + (i - 1) * 258, 332),
            FVector2D(246, 272), FLinearColor(0, 0, 0, 0), FLinearColor::Transparent, 1);
    }
	// Leave room for the full benefit and drawback in each editable UMG card.
	if (UCanvasPanelSlot* CardsSlot = Cast<UCanvasPanelSlot>(ConversationTree->FindWidget(TEXT("ResponseCards"))->Slot))
	{
		CardsSlot->SetPosition(FVector2D(460, 310));
		CardsSlot->SetSize(FVector2D(775, 294));
	}
	for (int32 i = 1; i <= 3; ++i)
	{
		if (UCanvasPanelSlot* ButtonSlot = Cast<UCanvasPanelSlot>(ConversationTree->FindWidget(FName(*FString::Printf(TEXT("ChoiceButton%d"), i)))->Slot))
		{
			ButtonSlot->SetPosition(FVector2D(460 + (i - 1) * 258, 310));
			ButtonSlot->SetSize(FVector2D(246, 294));
		}
		if (UBorder* Fill = Cast<UBorder>(ConversationTree->FindWidget(FName(*FString::Printf(TEXT("CardFill%d"), i)))))
			Fill->SetPadding(FMargin(14, 14));
		for (const TCHAR* Prefix : {TEXT("CardHint"), TEXT("CardCost")})
		{
			if (UTextBlock* Label = Cast<UTextBlock>(ConversationTree->FindWidget(FName(*FString::Printf(TEXT("%s%d"), Prefix, i)))))
			{
				Label->SetAutoWrapText(true);
				Label->SetWrapTextAt(210);
				if (UVerticalBoxSlot* Slot = Cast<UVerticalBoxSlot>(Label->Slot))
					Slot->SetPadding(FMargin(0, FCString::Strcmp(Prefix, TEXT("CardHint")) == 0 ? 16 : 10, 0, 0));
			}
		}
	}
    AddButton(TEXT("RefreshButton"), TEXT("RENOVAR"), FVector2D(740, 620), FVector2D(225, 50),
        FLinearColor(.9f, .31f, .02f), FLinearColor::White, 18);
    AddButton(TEXT("SkipButton"), TEXT("OMITIR"), FVector2D(985, 620), FVector2D(225, 50),
        FLinearColor(.06f, .055f, .045f), FLinearColor(.94f, .37f, .07f), 18);
    // Persist an asset-backed font. FCoreStyle's transient composite is not serialized.
    UObject* FontAsset = LoadObject<UObject>(nullptr, TEXT("/Engine/EngineFonts/Roboto.Roboto"));
    for (UWidgetBlueprint* BP : {Presentation, Conversation})
    {
        BP->Modify();
        BP->WidgetTree->Modify();
        TSet<FName> ActiveWidgetNames;
        BP->WidgetTree->ForEachWidget([BP, FontAsset, &ActiveWidgetNames](UWidget* Widget)
        {
            ActiveWidgetNames.Add(Widget->GetFName());
            if (!BP->WidgetVariableNameToGuidMap.Contains(Widget->GetFName()))
                BP->WidgetVariableNameToGuidMap.Add(Widget->GetFName(), FGuid::NewGuid());
            if (UTextBlock* Label = Cast<UTextBlock>(Widget))
            {
                FSlateFontInfo Font = Label->GetFont();
                if (!Font.FontObject) { Font.FontObject = FontAsset; Font.TypefaceFontName = TEXT("Bold"); Label->SetFont(Font); }
                if (Label->GetName().StartsWith(TEXT("Response")) || Label->GetName().StartsWith(TEXT("CardHint")) || Label->GetName().StartsWith(TEXT("CardCost")))
                {
                    Label->SetWrapTextAt(210);
                    Label->SetAutoWrapText(true);
                    Font.Size = Label->GetName().StartsWith(TEXT("Response")) ? 19 : 14;
                    Label->SetFont(Font);
                }
            }
        });
        for (auto It = BP->WidgetVariableNameToGuidMap.CreateIterator(); It; ++It)
            if (!ActiveWidgetNames.Contains(It.Key())) It.RemoveCurrent();
        FKismetEditorUtilities::CompileBlueprint(BP);
        BP->MarkPackageDirty();
    }
    return Presentation->Status != BS_Error && Conversation->Status != BS_Error;
}

bool UChopItPhoneWidgetEditorLibrary::ExportPhoneDialoguePreview()
{
    UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
    UClass* PresentationClass = LoadClass<UChopItPhonePresentationWidget>(nullptr,
        TEXT("/Game/ChopIt/Phone/WBP_PhonePresentation.WBP_PhonePresentation_C"));
    UChopItDialogueSequence* Sequence = LoadObject<UChopItDialogueSequence>(nullptr,
        TEXT("/Game/ChopIt/Phone/DA_PhoneDialogueDevil.DA_PhoneDialogueDevil"));
    UChopItPhoneVariantData* Variant = LoadObject<UChopItPhoneVariantData>(nullptr,
        TEXT("/Game/ChopIt/Phone/DA_PhoneDevil.DA_PhoneDevil"));
    if (!World || !PresentationClass || !Sequence || !Variant || !Sequence->WidgetClass) return false;
    UChopItPhonePresentationWidget* Portrait = CreateWidget<UChopItPhonePresentationWidget>(World, PresentationClass);
    UChopItPhoneConversationWidget* Conversation = CreateWidget<UChopItPhoneConversationWidget>(World, Sequence->WidgetClass);
    if (!Portrait || !Conversation) return false;
    FActorSpawnParameters Params;
    Params.ObjectFlags |= RF_Transient;
    AChopItMysteryPhone* Phone = World->SpawnActor<AChopItMysteryPhone>(FVector(0,0,100000), FRotator::ZeroRotator, Params);
    AChopItPhoneDialogueScene* Scene = World->SpawnActor<AChopItPhoneDialogueScene>(FVector(0,0,110000), FRotator::ZeroRotator, Params);
    const bool bInitialized = Scene && Phone && Scene->InitializeScene(Phone, Variant);
    if (!bInitialized) { if (Phone) Phone->Destroy(); if (Scene) Scene->Destroy(); return false; }
    FAssetCompilingManager::Get().FinishAllCompilation();
    if (GShaderCompilingManager) GShaderCompilingManager->FinishAllCompilation();
    World->SendAllEndOfFrameUpdates();
    FlushRenderingCommands();
    Scene->UpdatePresentation(0);
    FlushRenderingCommands();
    TArray<FLinearColor> CapturePixels;
    Scene->GetTexture()->GameThread_GetRenderTargetResource()->ReadLinearColorPixels(CapturePixels);
    float MinAlpha = 1, MaxLight = 0;
    for (const FLinearColor& Pixel : CapturePixels) { MinAlpha = FMath::Min(MinAlpha, Pixel.A); MaxLight = FMath::Max(MaxLight, Pixel.GetMax()); }
    UE_LOG(LogTemp, Display, TEXT("Phone preview capture: pixels=%d minAlpha=%f maxLight=%f"), CapturePixels.Num(), MinAlpha, MaxLight);
    UChopItPhonePresentationViewModel* VM = NewObject<UChopItPhonePresentationViewModel>();
    VM->SetPortraitMaterial(Scene->GetCompositeMaterial());
    VM->SetVariantName(Variant->DisplayName);
    VM->SetAccentColor(Variant->AccentColor);
    Portrait->SetViewModel(VM);
    TSharedRef<SWidget> PortraitSlate = Portrait->TakeWidget();
    TSharedRef<SWidget> ConversationSlate = Conversation->TakeWidget();
    const FChopItDialogueLine* Line = Sequence->FindLine(Sequence->EntryLineId);
    Conversation->ConfigureTheme(nullptr, true, 1);
    Conversation->ShowLine(*Line, FChopItDialogueMarkup::Compile(Line->Text.ToString()));
    Conversation->SetRevealCount(10000);
    TArray<FChopItDialogueChoiceView> Choices;
    for (const FChopItDialogueChoice& Choice : Line->Choices) Choices.Add({Choice.ChoiceId, Choice.Text, Choice.Description});
    Conversation->SetChoices(Choices, 0);
    TSharedRef<SOverlay> Overlay = SNew(SOverlay)
        + SOverlay::Slot()[PortraitSlate]
        + SOverlay::Slot()[ConversationSlate];
    Overlay->SlatePrepass();
    FWidgetRenderer* Renderer = new FWidgetRenderer(true);
    UTextureRenderTarget2D* Target = Renderer->DrawWidget(Overlay, FVector2D(1280,720));
    FlushRenderingCommands();
    FBufferArchive Png;
    const bool bSaved = FImageUtils::ExportRenderTarget2DAsPNG(Target, Png) &&
        FFileHelper::SaveArrayToFile(Png, *FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("PhoneDialoguePreview.png")));
    BeginCleanup(Renderer);
    Portrait->SetViewModel(nullptr);
    Scene->Destroy();
    Phone->Destroy();
    return bSaved;
}
