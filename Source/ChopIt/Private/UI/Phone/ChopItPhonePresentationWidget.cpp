#include "UI/Phone/ChopItPhonePresentationWidget.h"

#include "UI/Phone/ChopItPhonePresentationViewModel.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/ScaleBox.h"
#include "Components/TextBlock.h"

void UChopItPhonePresentationWidget::SetViewModel(UChopItPhonePresentationViewModel* InViewModel)
{
	if (ViewModel) ViewModel->RemoveAllFieldValueChangedDelegates(this);
	ViewModel = InViewModel;
	if (ViewModel)
	{
		using FDescriptor = UChopItPhonePresentationViewModel::FFieldNotificationClassDescriptor;
		const auto Bind = [this](UE::FieldNotification::FFieldId Field)
		{
			ViewModel->AddFieldValueChangedDelegate(Field,
				UChopItPhonePresentationViewModel::FFieldValueChangedDelegate::CreateUObject(
					this, &UChopItPhonePresentationWidget::OnFieldChanged));
		};
		Bind(FDescriptor::VariantName);
		Bind(FDescriptor::AccentColor);
		Bind(FDescriptor::PortraitMaterial);
	}
	ApplyViewModel();
}

TSharedRef<SWidget> UChopItPhonePresentationWidget::RebuildWidget()
{
	const TSharedRef<SWidget> Built = Super::RebuildWidget();
	ApplyViewModel();
	return Built;
}

void UChopItPhonePresentationWidget::OnFieldChanged(UObject* Object, UE::FieldNotification::FFieldId)
{
	if (Object == ViewModel) ApplyViewModel();
}

void UChopItPhonePresentationWidget::ApplyViewModel()
{
	if (!ViewModel || !Portrait) return;
	Portrait->SetBrushFromMaterial(ViewModel->PortraitMaterial);
}

void UChopItPhonePresentationWidget::NativeDestruct()
{
	if (ViewModel) ViewModel->RemoveAllFieldValueChangedDelegates(this);
	Super::NativeDestruct();
}
