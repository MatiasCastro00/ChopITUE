#include "UI/Phone/ChopItPhoneIndicatorBinding.h"
#include "UI/Phone/ChopItPhoneIndicatorConfig.h"
#include "UI/Phone/ChopItPhoneIndicatorViewModel.h"
#include "UI/Phone/ChopItPhoneIndicatorWidget.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChopItPhoneStage3Test, "ChopIt.Phone.Stage3.ProjectionAndMVVM",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FChopItPhoneStage3Test::RunTest(const FString& Parameters)
{
	const UChopItPhoneIndicatorConfig* Config = LoadObject<UChopItPhoneIndicatorConfig>(nullptr,
		TEXT("/Game/ChopIt/Phone/DA_PhoneIndicator.DA_PhoneIndicator"));
	if (!TestNotNull(TEXT("Indicator Data Asset"), Config)) return false;
	TestTrue(TEXT("Editable indicator config is valid"), Config->IsValidConfig());
	TestNotNull(TEXT("Editable widget Blueprint"), Config->WidgetClass.Get());
	TestNotNull(TEXT("Comic bubble sprite is assigned"), Config->BubbleTexture.Get());
	TestNotNull(TEXT("RING sprite is assigned"), Config->RingTexture.Get());
	// A 140 cm frame projects to half as many pixels at twice the camera depth.
	const float NearScale = FChopItPhoneIndicatorProjection::ResolveWorldScale(120.0f, 240.0f, 1.0f);
	const float FarScale = FChopItPhoneIndicatorProjection::ResolveWorldScale(60.0f, 240.0f, 1.0f);
	TestTrue(TEXT("Doubling camera depth halves the comic without a minimum screen size"),
		FMath::IsNearlyEqual(FarScale, NearScale * 0.5f));
	const float LowDpiScale = FChopItPhoneIndicatorProjection::ResolveWorldScale(120.0f, 240.0f, 0.75f);
	TestTrue(TEXT("DPI does not change the projected width in pixels"),
		FMath::IsNearlyEqual(LowDpiScale * 240.0f * 0.75f, 120.0f));
	const float WideWidgetScale = FChopItPhoneIndicatorProjection::ResolveWorldScale(120.0f, 480.0f, 1.0f);
	TestTrue(TEXT("Changing the editable UMG frame preserves its configured world width"),
		FMath::IsNearlyEqual(WideWidgetScale * 480.0f, 120.0f));
	const auto WorldAnchor = FChopItPhoneIndicatorProjection::Resolve(FVector2D(1920, 1080),
		FVector2D(960, 470), true, FVector(0, 0, 1), 100, FVector2D::ZeroVector);
	TestTrue(TEXT("Projected world anchor has no additional pixel height offset"),
		WorldAnchor.Position.Equals(FVector2D(960, 470), 0.01));

	const FVector2D View(1920, 1080);
	const auto Center = FChopItPhoneIndicatorProjection::Resolve(View, FVector2D(960, 540), true,
		FVector(0, 0, 1), 100, FVector2D(0, -90));
	TestFalse(TEXT("Visible phone stays in screen"), Center.bOffscreen);
	TestTrue(TEXT("Bubble follows phone with offset"), Center.Position.Equals(FVector2D(960, 450), 0.1));
	const auto Right = FChopItPhoneIndicatorProjection::Resolve(View, FVector2D(2500, 540), true,
		FVector(1, 0, 1), 100, FVector2D::ZeroVector);
	TestTrue(TEXT("Offscreen phone clamps to right edge"), Right.bOffscreen && FMath::IsNearlyEqual(Right.Position.X, 1820));
	const auto BehindLeft = FChopItPhoneIndicatorProjection::Resolve(View, FVector2D::ZeroVector, false,
		FVector(-1, 0, -1), 100, FVector2D::ZeroVector);
	TestTrue(TEXT("Behind-camera phone points left"), BehindLeft.bOffscreen && FMath::IsNearlyEqual(BehindLeft.Position.X, 100));
	const auto BehindUpRight = FChopItPhoneIndicatorProjection::Resolve(FVector2D(1280, 720), FVector2D::ZeroVector,
		false, FVector(1, 1, -1), 70, FVector2D::ZeroVector);
	TestTrue(TEXT("Aspect ratio and behind-camera direction respected"), BehindUpRight.bOffscreen &&
		BehindUpRight.Position.X > 640 && BehindUpRight.Position.Y < 360 &&
		BehindUpRight.Position.X <= 1210.1 && BehindUpRight.Position.Y >= 69.9);

	UChopItPhoneIndicatorViewModel* VM = NewObject<UChopItPhoneIndicatorViewModel>();
	int32 OpacityEvents = 0;
	VM->AddFieldValueChangedDelegate(UChopItPhoneIndicatorViewModel::FFieldNotificationClassDescriptor::Opacity,
		UChopItPhoneIndicatorViewModel::FFieldValueChangedDelegate::CreateLambda(
			[&OpacityEvents](UObject*, UE::FieldNotification::FFieldId) { ++OpacityEvents; }));
	VM->SetOpacity(0.5f);
	VM->SetOpacity(0.5f);
	TestEqual(TEXT("MVVM notifies only changed values"), OpacityEvents, 1);
	return true;
}
#endif
