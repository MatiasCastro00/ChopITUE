#include "UI/Phone/ChopItPhonePromptWidget.h"

#include "UI/Phone/ChopItPhonePromptViewModel.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/Image.h"
#include "Math/UnrealMathUtility.h"

void UChopItPhonePromptWidget::SetViewModel(UChopItPhonePromptViewModel* InViewModel)
{
	if (ViewModel) ViewModel->RemoveAllFieldValueChangedDelegates(this);
	ViewModel = InViewModel;
	if (ViewModel)
	{
		using FDescriptor = UChopItPhonePromptViewModel::FFieldNotificationClassDescriptor;
		ViewModel->AddFieldValueChangedDelegate(FDescriptor::bVisible,
			UChopItPhonePromptViewModel::FFieldValueChangedDelegate::CreateUObject(
				this, &UChopItPhonePromptWidget::OnFieldChanged));
	}
	ApplyViewModel();
}

TSharedRef<SWidget> UChopItPhonePromptWidget::RebuildWidget()
{
	// Every visual is authored in WBP_PhoneInteractPrompt's Designer tree.
	const TSharedRef<SWidget> Built = Super::RebuildWidget();
	KeyGroup = WidgetTree ? Cast<UCanvasPanel>(WidgetTree->FindWidget(TEXT("KeyGroup"))) : nullptr;
	KeyPlate = WidgetTree ? Cast<UImage>(WidgetTree->FindWidget(TEXT("KeyPlate"))) : nullptr;
	KeyGlyph = WidgetTree ? Cast<UImage>(WidgetTree->FindWidget(TEXT("KeyGlyph"))) : nullptr;
	RayLeft = WidgetTree ? Cast<UImage>(WidgetTree->FindWidget(TEXT("RayLeft"))) : nullptr;
	RayCenter = WidgetTree ? Cast<UImage>(WidgetTree->FindWidget(TEXT("RayCenter"))) : nullptr;
	RayRight = WidgetTree ? Cast<UImage>(WidgetTree->FindWidget(TEXT("RayRight"))) : nullptr;
	ApplyViewModel();
	return Built;
}

void UChopItPhonePromptWidget::OnFieldChanged(UObject* Object, UE::FieldNotification::FFieldId FieldId)
{
	if (Object == ViewModel) ApplyViewModel();
}

void UChopItPhonePromptWidget::ApplyViewModel()
{
	if (!KeyGroup || !KeyPlate || !KeyGlyph || !RayLeft || !RayCenter || !RayRight) return;
	const bool bShow = ViewModel && ViewModel->bVisible;
	if (bShow && !bWasVisible) AnimationTime = 0.0f;
	bWasVisible = bShow;
	SetVisibility(bShow ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
}

void UChopItPhonePromptWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (!bWasVisible || !KeyGroup || !RayLeft || !RayCenter || !RayRight) return;
	AnimationTime += InDeltaTime;
	const float Reveal = FMath::Clamp(AnimationTime / 0.22f, 0.0f, 1.0f);
	const float Pop = 1.0f + 0.20f * FMath::Sin(Reveal * PI) * (1.0f - Reveal);
	const float Wave = FMath::Sin(AnimationTime * 2.0f * PI * BobFrequency);
	KeyGroup->SetRenderTranslation(FVector2D(0.0f, -BobHeight * Wave));
	KeyGroup->SetRenderScale(FVector2D(Pop * (1.0f + 0.035f * Wave)));
	UImage* Rays[] = { RayLeft, RayCenter, RayRight };
	for (int32 Index = 0; Index < 3; ++Index)
	{
		const float Phase = AnimationTime * 8.0f - Index * 0.65f;
		const float Flash = 0.5f + 0.5f * FMath::Sin(Phase);
		Rays[Index]->SetRenderTranslation(FVector2D(0.0f, -BobHeight * Wave));
		Rays[Index]->SetRenderOpacity(Reveal * (0.55f + Flash * 0.45f));
		Rays[Index]->SetRenderScale(FVector2D(0.92f + Flash * 0.13f));
	}
}

void UChopItPhonePromptWidget::NativeDestruct()
{
	if (ViewModel) ViewModel->RemoveAllFieldValueChangedDelegates(this);
	Super::NativeDestruct();
}
