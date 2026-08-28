#include "ChopItDeveloperSettings.h"
#include "Combat/ChopItDamageTypes.h"
#include "Combat/ChopItHealthComponent.h"
#include "Feedback/ChopItHitFeedbackComponent.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FChopItPhase13DamageFeedbackContractTest,
	"ChopIt.Phase13.DamageFeedbackContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FChopItPhase13DamageFeedbackContractTest::RunTest(const FString& Parameters)
{
	UChopItHealthComponent* Health = NewObject<UChopItHealthComponent>();
	float ReportedDamage = 0.0f;
	bool bReportedCritical = false;
	int32 EventCount = 0;
	Health->OnDamageReceived.AddLambda(
		[&](const float Damage, const bool bCritical, AActor*, const FVector&)
		{
			ReportedDamage = Damage;
			bReportedCritical = bCritical;
			++EventCount;
		});

	FChopItDamageSpec Damage;
	Damage.BaseDamage = 12.0f;
	Damage.bCritical = true;
	Damage.CriticalMultiplier = 2.0f;
	Health->ApplyDamage(Damage, nullptr);
	TestEqual(TEXT("One presentation event is emitted per accepted hit"), EventCount, 1);
	TestEqual(TEXT("Presentation receives authoritative applied damage"), ReportedDamage, 24.0f);
	TestTrue(TEXT("Critical flag reaches presentation"), bReportedCritical);

	UChopItHitFeedbackComponent* Feedback = NewObject<UChopItHitFeedbackComponent>();
	TestNotNull(TEXT("Reusable feedback component can be constructed"), Feedback);
	const UChopItDeveloperSettings* Settings = GetDefault<UChopItDeveloperSettings>();
	TestNotNull(TEXT("Accessibility settings are available"), Settings);
	TestTrue(TEXT("Camera strength has a safe default"), Settings && Settings->CameraShakeStrength >= 0.0f && Settings->CameraShakeStrength <= 1.0f);
	TestTrue(TEXT("Effects density has a safe default"), Settings && Settings->EffectsDensity >= 0.0f && Settings->EffectsDensity <= 1.0f);
	return true;
}

#endif
