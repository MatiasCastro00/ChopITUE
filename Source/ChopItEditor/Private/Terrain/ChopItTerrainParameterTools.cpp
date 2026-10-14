#include "Terrain/ChopItTerrainParameterTools.h"
#include "Terrain/ChopItTerrainEditorSettings.h"
#include "UObject/UnrealType.h"

namespace
{
struct FParameter
{
	FString Path;
	FString Category;
	FProperty* Property = nullptr;
	void* Value = nullptr;
};

FString CategoryOf(const FProperty* Property)
{
	FString Category = Property->GetMetaData(TEXT("Category"));
	int32 Separator = INDEX_NONE;
	if (Category.FindLastChar(TEXT('|'), Separator)) Category.RightChopInline(Separator + 1);
	return Category.IsEmpty() ? TEXT("Other") : Category;
}

void Gather(UChopItTerrainEditorSettings& Settings, TArray<FParameter>& Out)
{
	for (TFieldIterator<FProperty> It(FChopItTerrainSettings::StaticStruct()); It; ++It)
	{
		FProperty* Property = *It;
		if (Property->GetFName() == GET_MEMBER_NAME_CHECKED(FChopItTerrainSettings, Regions))
		{
			for (TFieldIterator<FProperty> RegionIt(FChopItTerrainRegionSettings::StaticStruct()); RegionIt; ++RegionIt)
			{
				FProperty* Inner = *RegionIt;
				Out.Add({ FString::Printf(TEXT("Terrain.Regions.%s"), *Inner->GetName()), CategoryOf(Inner), Inner,
					Inner->ContainerPtrToValuePtr<void>(&Settings.Terrain.Regions) });
			}
		}
		else Out.Add({ FString::Printf(TEXT("Terrain.%s"), *Property->GetName()), CategoryOf(Property), Property,
			Property->ContainerPtrToValuePtr<void>(&Settings.Terrain) });
	}
	for (TFieldIterator<FProperty> It(FChopItTerrainMeshSettings::StaticStruct()); It; ++It)
	{
		FProperty* Property = *It;
		Out.Add({ FString::Printf(TEXT("Mesh.%s"), *Property->GetName()), CategoryOf(Property), Property,
			Property->ContainerPtrToValuePtr<void>(&Settings.Mesh) });
	}
}

FParameter* Find(TArray<FParameter>& Parameters, const FString& Path)
{
	return Parameters.FindByPredicate([&](const FParameter& Item) { return Item.Path == Path; });
}

bool Valid(const UChopItTerrainEditorSettings& Settings, FString& Error)
{
	if (!Settings.Terrain.Validate(Error)) return false;
	if (Settings.Mesh.PolygonDetail < 0 || Settings.Mesh.PolygonDetail > 100
		|| Settings.Mesh.CellsPerChunk < 8 || Settings.Mesh.CellsPerChunk > 128
		|| (Settings.Mesh.Shading != EChopItTerrainShading::Smooth && Settings.Mesh.Shading != EChopItTerrainShading::Flat))
	{ Error = TEXT("Invalid mesh controls."); return false; }
	return true;
}

void Persist(UChopItTerrainEditorSettings& Settings)
{
	if (Settings.HasAnyFlags(RF_ClassDefaultObject)) Settings.SaveConfig();
}

bool RandomizeValue(FParameter& Parameter, FRandomStream& Random)
{
	if (FBoolProperty* Bool = CastField<FBoolProperty>(Parameter.Property))
	{ Bool->SetPropertyValue(Parameter.Value, !Bool->GetPropertyValue(Parameter.Value)); return true; }
	if (FEnumProperty* Enum = CastField<FEnumProperty>(Parameter.Property))
	{
		const int64 Current = Enum->GetUnderlyingProperty()->GetSignedIntPropertyValue(Parameter.Value);
		Enum->GetUnderlyingProperty()->SetIntPropertyValue(Parameter.Value, Current == 0 ? int64(1) : int64(0));
		return true;
	}
	FNumericProperty* Number = CastField<FNumericProperty>(Parameter.Property);
	if (!Number) return false; // Arrays and compound curves are resettable, but not randomized blindly.
	if (Parameter.Path == TEXT("Terrain.Seed"))
	{ Number->SetIntPropertyValue(Parameter.Value, int64(Random.GetUnsignedInt())); return true; }
	double Low = 0, High = 0;
	const FString UIlow = Parameter.Property->GetMetaData(TEXT("UIMin"));
	const FString UIhigh = Parameter.Property->GetMetaData(TEXT("UIMax"));
	const FString ClampLow = Parameter.Property->GetMetaData(TEXT("ClampMin"));
	const FString ClampHigh = Parameter.Property->GetMetaData(TEXT("ClampMax"));
	if (!UIlow.IsEmpty() && !UIhigh.IsEmpty())
	{ Low = FCString::Atod(*UIlow); High = FCString::Atod(*UIhigh); }
	else if (!ClampLow.IsEmpty() && !ClampHigh.IsEmpty())
	{ Low = FCString::Atod(*ClampLow); High = FCString::Atod(*ClampHigh); }
	else return false;
	if (Parameter.Path == TEXT("Terrain.SamplesX") || Parameter.Path == TEXT("Terrain.SamplesY"))
	{ Low = 33; High = 513; }
	if (High > 10000 && UIhigh.IsEmpty())
	{
		const double Current = Number->IsFloatingPoint() ? Number->GetFloatingPointPropertyValue(Parameter.Value)
			: double(Number->GetSignedIntPropertyValue(Parameter.Value));
		Low = FMath::Clamp(Current * 0.55, Low, High);
		High = FMath::Clamp(Current * 1.45, Low + 1.0, High);
	}
	if (!FMath::IsFinite(Low) || !FMath::IsFinite(High) || High <= Low) return false;
	if (Number->IsFloatingPoint()) Number->SetFloatingPointPropertyValue(Parameter.Value, FMath::Lerp(Low, High, double(Random.FRand())));
	else Number->SetIntPropertyValue(Parameter.Value, int64(FMath::RoundToInt(FMath::Lerp(Low, High, double(Random.FRand())))));
	return true;
}
}

TArray<FString> FChopItTerrainParameterTools::GetParameterPaths()
{
	UChopItTerrainEditorSettings* Temporary = NewObject<UChopItTerrainEditorSettings>(GetTransientPackage());
	TArray<FParameter> Parameters;
	Gather(*Temporary, Parameters);
	TArray<FString> Paths;
	for (const FParameter& Item : Parameters) Paths.Add(Item.Path);
	Paths.Sort();
	return Paths;
}

TArray<FString> FChopItTerrainParameterTools::GetCategories()
{
	UChopItTerrainEditorSettings* Temporary = NewObject<UChopItTerrainEditorSettings>(GetTransientPackage());
	TArray<FParameter> Parameters;
	Gather(*Temporary, Parameters);
	TSet<FString> Unique;
	for (const FParameter& Item : Parameters) Unique.Add(Item.Category);
	TArray<FString> Categories = Unique.Array();
	Categories.Sort();
	Categories.Insert(TEXT("All"), 0);
	return Categories;
}

bool FChopItTerrainParameterTools::IsLocked(const UChopItTerrainEditorSettings& Settings, const FString& Path)
{
	return Settings.LockedParameters.Contains(Path);
}

bool FChopItTerrainParameterTools::IsDefault(const UChopItTerrainEditorSettings& Settings, const FString& Path)
{
	static const FChopItTerrainSettings DefaultTerrain;
	static const FChopItTerrainMeshSettings DefaultMesh;
	const UScriptStruct* Struct = nullptr;
	const void* Current = nullptr;
	const void* Default = nullptr;
	FString Name;
	if (Path.StartsWith(TEXT("Terrain.Regions.")))
	{
		Struct = FChopItTerrainRegionSettings::StaticStruct();
		Current = &Settings.Terrain.Regions; Default = &DefaultTerrain.Regions;
		Name = Path.RightChop(16);
	}
	else if (Path.StartsWith(TEXT("Terrain.")))
	{
		Struct = FChopItTerrainSettings::StaticStruct();
		Current = &Settings.Terrain; Default = &DefaultTerrain;
		Name = Path.RightChop(8);
	}
	else if (Path.StartsWith(TEXT("Mesh.")))
	{
		Struct = FChopItTerrainMeshSettings::StaticStruct();
		Current = &Settings.Mesh; Default = &DefaultMesh;
		Name = Path.RightChop(5);
	}
	if (!Struct) return true;
	const FProperty* Property = Struct->FindPropertyByName(*Name);
	return !Property || Property->Identical(Property->ContainerPtrToValuePtr<void>(Current),
		Property->ContainerPtrToValuePtr<void>(Default));
}

bool FChopItTerrainParameterTools::ToggleLock(UChopItTerrainEditorSettings& Settings, const FString& Path, FString& OutError)
{
	TArray<FParameter> Parameters;
	Gather(Settings, Parameters);
	if (!Find(Parameters, Path)) { OutError = TEXT("Choose a valid parameter."); return false; }
	Settings.Modify();
	if (IsLocked(Settings, Path)) Settings.LockedParameters.Remove(Path);
	else Settings.LockedParameters.Add(Path);
	Persist(Settings);
	return true;
}

bool FChopItTerrainParameterTools::ResetParameter(UChopItTerrainEditorSettings& Settings, const FString& Path, FString& OutError)
{
	if (IsLocked(Settings, Path)) { OutError = TEXT("Unlock this parameter before resetting it."); return false; }
	UChopItTerrainEditorSettings* Candidate = NewObject<UChopItTerrainEditorSettings>(GetTransientPackage());
	Candidate->Terrain = Settings.Terrain; Candidate->Mesh = Settings.Mesh;
	UChopItTerrainEditorSettings* Defaults = NewObject<UChopItTerrainEditorSettings>(GetTransientPackage());
	Defaults->Terrain = FChopItTerrainSettings();
	Defaults->Mesh = FChopItTerrainMeshSettings();
	TArray<FParameter> CandidateParams, DefaultParams;
	Gather(*Candidate, CandidateParams); Gather(*Defaults, DefaultParams);
	FParameter* Destination = Find(CandidateParams, Path);
	FParameter* Source = Find(DefaultParams, Path);
	if (!Destination || !Source) { OutError = TEXT("Choose a valid parameter."); return false; }
	Destination->Property->CopyCompleteValue(Destination->Value, Source->Value);
	if (!Valid(*Candidate, OutError)) return false;
	Settings.Modify();
	Settings.Terrain = Candidate->Terrain; Settings.Mesh = Candidate->Mesh;
	Persist(Settings);
	return true;
}

bool FChopItTerrainParameterTools::Randomize(UChopItTerrainEditorSettings& Settings, const FString& Category, FString& OutError)
{
	const TArray<FString> Categories = GetCategories();
	if (!Categories.Contains(Category)) { OutError = TEXT("Choose a valid category."); return false; }
	UChopItTerrainEditorSettings* Candidate = NewObject<UChopItTerrainEditorSettings>(GetTransientPackage());
	Candidate->Terrain = Settings.Terrain; Candidate->Mesh = Settings.Mesh;
	if (!Valid(*Candidate, OutError)) return false;
	TArray<FParameter> Parameters;
	Gather(*Candidate, Parameters);
	FRandomStream Random(int32(FGuid::NewGuid().A));
	int32 Changed = 0;
	for (FParameter& Parameter : Parameters)
	{
		if ((Category != TEXT("All") && Parameter.Category != Category) || IsLocked(Settings, Parameter.Path)) continue;
		for (int32 Attempt = 0; Attempt < 32; ++Attempt)
		{
			const FChopItTerrainSettings OldTerrain = Candidate->Terrain;
			const FChopItTerrainMeshSettings OldMesh = Candidate->Mesh;
			if (!RandomizeValue(Parameter, Random)) break;
			FString Error;
			const bool bChanged = !FChopItTerrainSettings::StaticStruct()->CompareScriptStruct(&OldTerrain, &Candidate->Terrain, 0)
				|| !FChopItTerrainMeshSettings::StaticStruct()->CompareScriptStruct(&OldMesh, &Candidate->Mesh, 0);
			if (bChanged && Valid(*Candidate, Error))
			{ ++Changed; break; }
			Candidate->Terrain = OldTerrain;
			Candidate->Mesh = OldMesh;
		}
	}
	if (Changed == 0) { OutError = TEXT("No unlocked randomizable controls in this category."); return false; }
	Settings.Modify();
	Settings.Terrain = Candidate->Terrain; Settings.Mesh = Candidate->Mesh;
	Persist(Settings);
	return true;
}

bool FChopItTerrainParameterTools::ApplyUnlocked(UChopItTerrainEditorSettings& Settings,
	const FChopItTerrainSettings& Terrain, const FChopItTerrainMeshSettings& Mesh, FString& OutError)
{
	UChopItTerrainEditorSettings* Candidate = NewObject<UChopItTerrainEditorSettings>(GetTransientPackage());
	Candidate->Terrain = Terrain; Candidate->Mesh = Mesh;
	TArray<FParameter> Destination, Source;
	Gather(*Candidate, Destination); Gather(Settings, Source);
	for (FParameter& Item : Destination)
		if (IsLocked(Settings, Item.Path))
			if (const FParameter* Old = Find(Source, Item.Path)) Item.Property->CopyCompleteValue(Item.Value, Old->Value);
	if (!Valid(*Candidate, OutError)) return false;
	Settings.Modify();
	Settings.Terrain = Candidate->Terrain; Settings.Mesh = Candidate->Mesh;
	Persist(Settings);
	return true;
}
