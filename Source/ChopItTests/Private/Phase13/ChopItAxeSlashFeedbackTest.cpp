#include "Components/StaticMeshComponent.h"
#include "Editor.h"
#include "Feedback/ChopItAxeSwingTrail.h"
#include "HAL/IConsoleManager.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/AutomationTest.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FChopItAxeSlashFeedbackContractTest,
	"ChopIt.Phase13.AxeSlashFeedbackContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FChopItAxeSlashFeedbackContractTest::RunTest(const FString& Parameters)
{
	const AChopItAxeSwingTrail* TrailCDO =
		AChopItAxeSwingTrail::StaticClass()->GetDefaultObject<AChopItAxeSwingTrail>();
	TestNotNull(TEXT("Axe slash actor has a class default object"), TrailCDO);
	if (!TrailCDO)
	{
		return false;
	}

	TInlineComponentArray<UStaticMeshComponent*> MeshLayers;
	TrailCDO->GetComponents(MeshLayers);
	TestEqual(TEXT("Slash is composed from three mesh layers"), MeshLayers.Num(), 3);
	for (const UStaticMeshComponent* Layer : MeshLayers)
	{
		TestNotNull(TEXT("Each slash layer has a fallback mesh"), Layer ? Layer->GetStaticMesh().Get() : nullptr);
		TestTrue(
			TEXT("Slash layers never participate in collision"),
			Layer && Layer->GetCollisionEnabled() == ECollisionEnabled::NoCollision);
		TestFalse(
			TEXT("Slash layers never generate overlap events"),
			Layer && Layer->GetGenerateOverlapEvents());
	}

	const UNiagaraComponent* NiagaraCDO = TrailCDO->FindComponentByClass<UNiagaraComponent>();
	TestNotNull(TEXT("Slash owns a Niagara detail component"), NiagaraCDO);
	if (NiagaraCDO)
	{
		TestFalse(TEXT("Niagara details start inactive"), NiagaraCDO->IsActive());
		TestTrue(
			TEXT("Niagara details have fixed local bounds"),
			NiagaraCDO->GetSystemFixedBounds().IsValid != 0);
	}

	IConsoleVariable* DebugCVar = IConsoleManager::Get().FindConsoleVariable(
		TEXT("ChopIt.Combat.DrawAttackDebug"));
	TestNotNull(TEXT("Attack debug cvar is registered"), DebugCVar);
	if (DebugCVar)
	{
		TestEqual(TEXT("Orange attack geometry is hidden by default"), DebugCVar->GetInt(), 0);
	}

	UWorld* EditorWorld = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	TestNotNull(TEXT("Editor world is available for slash asset integration"), EditorWorld);
	if (!EditorWorld)
	{
		return false;
	}

	const auto SpawnTrail = [EditorWorld]()
	{
		FActorSpawnParameters SpawnParameters;
		SpawnParameters.ObjectFlags |= RF_Transient;
		SpawnParameters.SpawnCollisionHandlingOverride =
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		return EditorWorld->SpawnActor<AChopItAxeSwingTrail>(
			AChopItAxeSwingTrail::StaticClass(),
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			SpawnParameters);
	};

	AChopItAxeSwingTrail* DisabledTrail = SpawnTrail();
	TestNotNull(TEXT("Density-zero slash cue can be initialized safely"), DisabledTrail);
	if (DisabledTrail)
	{
		DisabledTrail->InitializeTrail(FVector::ForwardVector, 500.0f, false, 0.0f);
		TestTrue(TEXT("Density zero hides all slash VFX"), DisabledTrail->IsHidden());
		DisabledTrail->Destroy();
	}

	AChopItAxeSwingTrail* PartialTrail = SpawnTrail();
	TestNotNull(TEXT("Partial-density slash cue can be spawned"), PartialTrail);
	if (PartialTrail)
	{
		PartialTrail->InitializeTrail(FVector::ForwardVector, 500.0f, false, 0.5f);
		TestFalse(TEXT("Partial density keeps the main slash visible"), PartialTrail->IsHidden());
		PartialTrail->Destroy();
	}

	AChopItAxeSwingTrail* LiveTrail = SpawnTrail();
	TestNotNull(TEXT("Full-density hit slash cue can be spawned"), LiveTrail);
	if (LiveTrail)
	{
		LiveTrail->InitializeTrail(FVector::ForwardVector, 500.0f, true, 1.0f);
		TMap<FString, FString> ExpectedMeshes;
		ExpectedMeshes.Add(TEXT("MainLayer"), TEXT("SM_AxeSlash_Main"));
		ExpectedMeshes.Add(TEXT("InnerLayer"), TEXT("SM_AxeSlash_Inner"));
		ExpectedMeshes.Add(TEXT("AfterimageLayer"), TEXT("SM_AxeSlash_Afterimage"));
		for (const TPair<FString, FString>& Expected : ExpectedMeshes)
		{
			const UStaticMeshComponent* Layer =
				FindObject<UStaticMeshComponent>(LiveTrail, *Expected.Key);
			TestNotNull(*FString::Printf(TEXT("%s component exists"), *Expected.Key), Layer);
			if (!Layer)
			{
				continue;
			}
			TestEqual(
				*FString::Printf(TEXT("%s uses its distinct authored geometry"), *Expected.Key),
				Layer->GetStaticMesh() ? Layer->GetStaticMesh()->GetName() : FString(),
				Expected.Value);
			const UMaterialInstanceDynamic* DynamicMaterial =
				Cast<UMaterialInstanceDynamic>(Layer->GetMaterial(0));
			TestNotNull(
				*FString::Printf(TEXT("%s receives an independent dynamic material"), *Expected.Key),
				DynamicMaterial);
			const UMaterial* BaseMaterial =
				DynamicMaterial ? DynamicMaterial->GetMaterial() : nullptr;
			const FString ExpectedMaterial = Expected.Key == TEXT("AfterimageLayer")
				? TEXT("M_AxeSlash_Afterimage")
				: TEXT("M_AxeSlash");
			TestEqual(
				*FString::Printf(TEXT("%s uses the expected blend-family material"), *Expected.Key),
				BaseMaterial ? BaseMaterial->GetName() : FString(),
				ExpectedMaterial);
		}

		const UNiagaraComponent* Details = LiveTrail->FindComponentByClass<UNiagaraComponent>();
		TestNotNull(TEXT("Runtime slash retains its Niagara component"), Details);
		const UNiagaraSystem* DetailsSystem = Details ? Details->GetAsset() : nullptr;
		TestNotNull(TEXT("Runtime slash loads NS_AxeSlash_Details"), DetailsSystem);
		if (DetailsSystem)
		{
			TestEqual(
				TEXT("Niagara system contains glint and brush-fragment emitters"),
				DetailsSystem->GetEmitterHandles().Num(),
				2);
			TestEqual(
				TEXT("Niagara system uses the authored detail asset"),
				DetailsSystem->GetName(),
				FString(TEXT("NS_AxeSlash_Details")));
		}
		LiveTrail->Destroy();
	}
	return true;
}

#endif
