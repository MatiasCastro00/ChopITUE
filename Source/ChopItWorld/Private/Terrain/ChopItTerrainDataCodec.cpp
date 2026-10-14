#include "Terrain/ChopItTerrainDataCodec.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"
#include "Misc/Compression.h"
#include "Misc/Crc.h"

namespace
{
constexpr uint32 Magic = 0x43485437; // CHT7
constexpr uint32 Version = 1;
constexpr int32 MaxSamples = 2049 * 2049;
}

void FChopItTerrainDataCodec::Transfer(FArchive& Ar, FChopItHeightField& Field)
{
	Ar << Field.SamplesX << Field.SamplesY << Field.Size << Field.MinHeight << Field.MaxHeight;
	Ar << Field.MeanHeight << Field.WaterLevel << Field.SubmergedFraction;
	const int32 Count = Field.SamplesX * Field.SamplesY;
	TArray<float>* Layers[] = { &Field.Heights, &Field.Maps.SlopeDegrees, &Field.Maps.Curvature,
		&Field.Maps.Walkability, &Field.Maps.Clearing, &Field.Maps.MoisturePotential, &Field.Maps.VegetationDensityPotential };
	for (TArray<float>* Layer : Layers)
	{
		if (Ar.IsLoading()) Layer->SetNumUninitialized(Count);
		Ar.Serialize(Layer->GetData(), int64(Count) * sizeof(float));
	}
	if (Ar.IsSaving())
	{
		for (EChopItTerrainRegion Region : Field.Maps.Regions) { uint8 Value = uint8(Region); Ar << Value; }
		for (const FChopItTerrainRegionBlend& Blend : Field.Maps.RegionBlends)
			for (float Weight : Blend.Weights) { float Value = Weight; Ar << Value; }
	}
	else
	{
		Field.Maps.Regions.SetNumUninitialized(Count);
		Field.Maps.RegionBlends.SetNumZeroed(Count);
		for (auto& Region : Field.Maps.Regions) { uint8 Value = 0; Ar << Value; Region = EChopItTerrainRegion(Value); }
		for (auto& Blend : Field.Maps.RegionBlends)
			for (float& Weight : Blend.Weights) Ar << Weight;
	}
}

bool FChopItTerrainDataCodec::Validate(const FChopItHeightField& Field)
{
	if (!Field.IsValid() || Field.SamplesX > 2049 || Field.SamplesY > 2049 || !Field.Maps.IsValid(Field.Heights.Num())) return false;
	if (!FMath::IsFinite(Field.MinHeight) || !FMath::IsFinite(Field.MaxHeight) || !FMath::IsFinite(Field.MeanHeight)
		|| !FMath::IsFinite(Field.WaterLevel) || !FMath::IsFinite(Field.SubmergedFraction)) return false;
	if (Field.MinHeight > Field.MaxHeight || Field.MeanHeight < Field.MinHeight - 0.01
		|| Field.MeanHeight > Field.MaxHeight + 0.01 || Field.SubmergedFraction < 0 || Field.SubmergedFraction > 1) return false;
	for (float Value : Field.Heights) if (!FMath::IsFinite(Value) || Value < Field.MinHeight || Value > Field.MaxHeight) return false;
	const TArray<float>* Layers[] = { &Field.Maps.SlopeDegrees, &Field.Maps.Curvature, &Field.Maps.Walkability,
		&Field.Maps.Clearing, &Field.Maps.MoisturePotential, &Field.Maps.VegetationDensityPotential };
	for (const TArray<float>* Layer : Layers) for (float Value : *Layer) if (!FMath::IsFinite(Value)) return false;
	for (float Value : Field.Maps.SlopeDegrees) if (Value < 0 || Value > 90) return false;
	for (int32 Layer = 2; Layer < UE_ARRAY_COUNT(Layers); ++Layer)
		for (float Value : *Layers[Layer]) if (Value < 0 || Value > 1) return false;
	for (EChopItTerrainRegion Region : Field.Maps.Regions) if (uint8(Region) >= uint8(EChopItTerrainRegion::Count)) return false;
	for (const FChopItTerrainRegionBlend& Blend : Field.Maps.RegionBlends)
	{
		float Sum = 0;
		for (float Value : Blend.Weights)
		{
			if (!FMath::IsFinite(Value) || Value < 0 || Value > 1) return false;
			Sum += Value;
		}
		if (!FMath::IsNearlyEqual(Sum, 1.0f, 0.0001f)) return false;
	}
	return true;
}

void FChopItTerrainDataCodec::Write(FArchive& Ar, const FChopItHeightField& Field)
{
	// FArchive's legacy interface takes mutable pointers; this writer only reads them.
	auto Scalar = [&Ar](auto Value) { Ar << Value; };
	Scalar(Field.SamplesX); Scalar(Field.SamplesY); Scalar(Field.Size);
	Scalar(Field.MinHeight); Scalar(Field.MaxHeight); Scalar(Field.MeanHeight);
	Scalar(Field.WaterLevel); Scalar(Field.SubmergedFraction);
	const TArray<float>* Layers[] = { &Field.Heights, &Field.Maps.SlopeDegrees, &Field.Maps.Curvature,
		&Field.Maps.Walkability, &Field.Maps.Clearing, &Field.Maps.MoisturePotential, &Field.Maps.VegetationDensityPotential };
	for (const TArray<float>* Layer : Layers)
		Ar.Serialize(const_cast<float*>(Layer->GetData()), int64(Layer->Num()) * sizeof(float));
	for (EChopItTerrainRegion Region : Field.Maps.Regions) Scalar(uint8(Region));
	for (const auto& Blend : Field.Maps.RegionBlends) for (float Weight : Blend.Weights) Scalar(Weight);
}

bool FChopItTerrainDataCodec::Encode(const FChopItHeightField& Field, TArray<uint8>& OutData, FString& OutError)
{
	OutError.Reset();
	if (!Validate(Field)) { OutError = TEXT("Invalid terrain field or maps."); return false; }
	TArray<uint8> Raw;
	Raw.Reserve(52 + Field.Heights.Num() * 57);
	FMemoryWriter Writer(Raw);
	Write(Writer, Field);
	int32 RawSize = Raw.Num();
	int32 Bound = FCompression::CompressMemoryBound(NAME_Zlib, RawSize);
	if (Bound <= 0) { OutError = TEXT("Compression size failed."); return false; }
	TArray<uint8> Packed;
	Packed.SetNumUninitialized(Bound);
	int32 PackedSize = Bound;
	if (!FCompression::CompressMemory(NAME_Zlib, Packed.GetData(), PackedSize, Raw.GetData(), RawSize))
	{ OutError = TEXT("Terrain compression failed."); return false; }
	OutData.Reset();
	OutData.Reserve(20 + PackedSize);
	FMemoryWriter Output(OutData);
	uint32 FileMagic = Magic, FileVersion = Version, Crc = FCrc::MemCrc32(Raw.GetData(), RawSize);
	Output << FileMagic << FileVersion;
	Output << RawSize << PackedSize << Crc;
	Output.Serialize(Packed.GetData(), PackedSize);
	return true;
}

bool FChopItTerrainDataCodec::Decode(const TArray<uint8>& Data, FChopItHeightField& OutField, FString& OutError)
{
	OutError.Reset();
	if (Data.Num() < 20) { OutError = TEXT("Terrain snapshot is incomplete."); return false; }
	FMemoryReader Input(Data);
	uint32 FileMagic = 0, FileVersion = 0, Crc = 0;
	int32 RawSize = 0, PackedSize = 0;
	Input << FileMagic << FileVersion << RawSize << PackedSize << Crc;
	// Eight float maps, seven blend weights and overhead per sample; cap before allocating.
	if (FileMagic != Magic || FileVersion != Version || RawSize < 52 + 57 * 4 || RawSize > 52 + MaxSamples * 57
		|| PackedSize <= 0 || PackedSize != Data.Num() - 20)
	{ OutError = TEXT("Unsupported or invalid terrain snapshot."); return false; }
	TArray<uint8> Raw;
	Raw.SetNumUninitialized(RawSize);
	if (!FCompression::UncompressMemory(NAME_Zlib, Raw.GetData(), RawSize, Data.GetData() + 20, PackedSize)
		|| FCrc::MemCrc32(Raw.GetData(), RawSize) != Crc)
	{ OutError = TEXT("Terrain snapshot failed decompression or checksum."); return false; }
	FMemoryReader Reader(Raw);
	FChopItHeightField Candidate;
	Reader << Candidate.SamplesX << Candidate.SamplesY;
	if (Candidate.SamplesX < 2 || Candidate.SamplesY < 2 || Candidate.SamplesX > 2049 || Candidate.SamplesY > 2049)
	{ OutError = TEXT("Terrain snapshot dimensions are invalid."); return false; }
	if (RawSize != 52 + int64(Candidate.SamplesX) * Candidate.SamplesY * 57)
	{ OutError = TEXT("Terrain snapshot length does not match its dimensions."); return false; }
	Reader.Seek(0);
	Transfer(Reader, Candidate);
	if (Reader.IsError() || Reader.Tell() != Raw.Num() || !Validate(Candidate))
	{ OutError = TEXT("Terrain snapshot contents are invalid."); return false; }
	OutField = MoveTemp(Candidate);
	return true;
}
