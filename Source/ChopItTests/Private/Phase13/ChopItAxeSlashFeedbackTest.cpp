#include "Components/StaticMeshComponent.h"
#include "Editor.h"
#include "Feedback/ChopItAxeSwingTrail.h"
#include "HAL/IConsoleManager.h"


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
	TestEqual(TEXT("Legacy mesh layers are replaced by Niagara"), MeshLayers.Num(), 0);

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
		const UNiagaraComponent* Details = LiveTrail->FindComponentByClass<UNiagaraComponent>();
		TestNotNull(TEXT("Runtime slash retains its Niagara component"), Details);
		const UNiagaraSystem* DetailsSystem = Details ? Details->GetAsset() : nullptr;
		TestNotNull(TEXT("Runtime slash loads NS_AxeSlash"), DetailsSystem);
		if (DetailsSystem)
		{
			TestEqual(
				TEXT("Niagara system contains the slash, edge and spark emitters"),
				DetailsSystem->GetEmitterHandles().Num(),
				3);
			TestEqual(
				TEXT("Niagara system uses the authored detail asset"),
				DetailsSystem->GetName(),
				FString(TEXT("NS_AxeSlash")));
		}
		LiveTrail->Destroy();
	}
	return true;
}

#endif

