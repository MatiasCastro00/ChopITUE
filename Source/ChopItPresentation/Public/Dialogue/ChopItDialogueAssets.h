#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Fonts/SlateFontInfo.h"
#include "Dialogue/ChopItDialogueTypes.h"
#include "ChopItDialogueAssets.generated.h"

class UCameraShakeAsset;
class UChopItCameraCue;
class UChopItCameraEffectPreset;
class USoundBase;
class UTexture2D;

UENUM(BlueprintType)
enum class EChopItDialogueCameraActionKind : uint8
{
	Cue,
	Effect,
	Shake
};

USTRUCT(BlueprintType)
struct CHOPITPRESENTATION_API FChopItDialogueCameraAction
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Camera") EChopItDialogueCameraActionKind Kind = EChopItDialogueCameraActionKind::Cue;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Camera") TSoftObjectPtr<UChopItCameraCue> Cue;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Camera") TSoftObjectPtr<UChopItCameraEffectPreset> Effect;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Camera") TSoftObjectPtr<UCameraShakeAsset> Shake;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Camera") FName AnchorBinding = TEXT("CameraAnchor");
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Camera") FName SubjectBinding = TEXT("Speaker");
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Camera", meta=(ClampMin="-1.0")) float DurationOverride = -1.0f;
	/** Negative keeps the cue asset's FOV; positive values allow dialogue beats to dolly optically in or out. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Camera", meta=(ClampMin="-1.0", ClampMax="170.0")) float FieldOfViewOverride = -1.0f;
	/** Negative keeps the cue asset transition; non-negative values override it for this beat. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Camera", meta=(ClampMin="-1.0")) float BlendInTimeOverride = -1.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Camera", meta=(ClampMin="0.0")) float Scale = 1.0f;
	/** Shake actions with this flag keep looping until the dialogue advances away from the current line. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Camera") bool bSustainUntilLineEnds = false;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Camera") bool bPersistUntilDialogueEnds = false;
};

UCLASS(BlueprintType)
class CHOPITPRESENTATION_API UChopItDialogueTheme : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Colors") FLinearColor PanelColor = FLinearColor(0.055f, 0.035f, 0.02f, 0.97f);
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Colors") FLinearColor BorderColor = FLinearColor(1.0f, 0.32f, 0.035f, 1.0f);
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Colors") FLinearColor TextColor = FLinearColor(0.96f, 0.88f, 0.69f, 1.0f);
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Colors") FLinearColor ChoiceColor = FLinearColor(0.18f, 0.11f, 0.055f, 0.98f);
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Colors") FLinearColor ChoiceSelectedColor = FLinearColor(0.85f, 0.22f, 0.025f, 1.0f);
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Typography") FSlateFontInfo BodyFont;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Typography") FSlateFontInfo NameFont;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Timing", meta=(ClampMin="0.0")) float EnterDuration = 0.18f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Timing", meta=(ClampMin="0.0")) float ExitDuration = 0.22f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Camera") TMap<FName, FChopItDialogueCameraAction> CameraActions;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Audio") TMap<FName, TSoftObjectPtr<USoundBase>> Sounds;
};

UCLASS(BlueprintType)
class CHOPITPRESENTATION_API UChopItDialogueSpeakerDefinition : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Speaker") FName SpeakerId;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Speaker") FText DisplayName;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Speaker") FLinearColor AccentColor = FLinearColor::White;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Speaker") EChopItDialoguePortraitSide PortraitSide = EChopItDialoguePortraitSide::Left;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Portrait") FName NeutralExpression = TEXT("Neutral");
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Portrait") TMap<FName, TSoftObjectPtr<UTexture2D>> Portraits;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Audio") TSoftObjectPtr<USoundBase> TypewriterBlip;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Audio", meta=(ClampMin="1")) int32 BlipCadence = 2;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Audio", meta=(ClampMin="0.5", ClampMax="2.0")) float MinBlipPitch = 0.94f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Audio", meta=(ClampMin="0.5", ClampMax="2.0")) float MaxBlipPitch = 1.06f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Audio") bool bPlayBlipsUnderVoice = false;

	UTexture2D* ResolvePortrait(FName Expression) const;
};

USTRUCT(BlueprintType)
struct CHOPITPRESENTATION_API FChopItDialogueChoice
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Choice") FName ChoiceId;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Choice") FText Text;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Choice") FName NextLineId;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Choice") FGameplayTag EventTag;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Choice") FGameplayTagContainer RequiredTags;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Choice") FGameplayTagContainer BlockedTags;

	bool IsAvailable(const FGameplayTagContainer& ContextTags) const
	{
		return ContextTags.HasAll(RequiredTags) && !ContextTags.HasAny(BlockedTags);
	}
};

USTRUCT(BlueprintType)
struct CHOPITPRESENTATION_API FChopItDialogueLine
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Line") FName LineId;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Line") TObjectPtr<UChopItDialogueSpeakerDefinition> Speaker;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Line", meta=(MultiLine="true")) FText Text;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Line") FName Expression = TEXT("Neutral");
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Line") FName NextLineId;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Line") TArray<FChopItDialogueChoice> Choices;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Line", meta=(ClampMin="0.05", ClampMax="10.0")) float TypewriterSpeed = 1.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Line") bool bAutoAdvance = false;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Line", meta=(ClampMin="0.0")) float AutoAdvanceDelay = 0.65f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Line") TSoftObjectPtr<USoundBase> Voice;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Line") FName CameraAction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Events") FGameplayTag StartEvent;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Events") FGameplayTag EndEvent;
};

UCLASS(BlueprintType)
class CHOPITPRESENTATION_API UChopItDialogueSequence : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dialogue") FName DialogueId;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dialogue") TObjectPtr<UChopItDialogueTheme> Theme;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dialogue") FName EntryLineId;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dialogue") TArray<FChopItDialogueLine> Lines;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mode") bool bPauseWorld = true;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mode") bool bBlockGameplayInput = true;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mode") bool bCanCancel = false;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mode", meta=(ClampMin="1", ClampMax="4096")) int32 VisitLimit = 512;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Events") FGameplayTag StartEvent;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Events") FGameplayTag EndEvent;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
	const FChopItDialogueLine* FindLine(FName LineId) const;
	bool ValidateSequence(TArray<FText>& OutErrors) const;
#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif
};
