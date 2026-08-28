#include "Feedback/ChopItDamageNumber.h"

#include "Components/TextRenderComponent.h"
#include "Kismet/GameplayStatics.h"

AChopItDamageNumber::AChopItDamageNumber()
{
	PrimaryActorTick.bCanEverTick = true;
	SetActorEnableCollision(false);
	TextRender = CreateDefaultSubobject<UTextRenderComponent>(TEXT("DamageText"));
	SetRootComponent(TextRender);
	TextRender->SetHorizontalAlignment(EHTA_Center);
	TextRender->SetVerticalAlignment(EVRTA_TextCenter);
	TextRender->SetWorldSize(38.0f);
	TextRender->SetCastShadow(false);
	TextRender->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AChopItDamageNumber::InitializeDamageNumber(const float Damage, const bool bCritical)
{
	bInitialized = true;
	SpawnLocation = GetActorLocation() + FVector(FMath::FRandRange(-14.0f, 14.0f), FMath::FRandRange(-14.0f, 14.0f), 104.0f);
	Drift = FVector(FMath::FRandRange(-18.0f, 18.0f), FMath::FRandRange(-18.0f, 18.0f), 78.0f);
	BaseScale = (bCritical ? 1.34f : 1.0f) * FMath::FRandRange(0.91f, 1.10f);
	RollOffset = FMath::FRandRange(-8.0f, 8.0f);
	StartColor = bCritical ? FLinearColor(1.0f, 0.56f, 0.04f) : FLinearColor(1.0f, 0.22f, 0.10f);
	TextRender->SetText(bCritical ? FText::FromString(FString::Printf(TEXT("CRIT! %.0f"), Damage)) : FText::AsNumber(FMath::RoundToInt(Damage)));
	TextRender->SetWorldSize(bCritical ? 48.0f : 38.0f);
}

void AChopItDamageNumber::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!bInitialized)
	{
		return;
	}
	Age += DeltaSeconds;
	if (Age >= Lifetime)
	{
		Destroy();
		return;
	}

	float Scale = BaseScale;
	float Alpha = 1.0f;
	FLinearColor Color = StartColor;
	if (Age < PopDuration)
	{
		const float PopAlpha = FMath::Clamp(Age / PopDuration, 0.0f, 1.0f);
		const float Growth = FMath::InterpEaseOut(0.62f, 1.18f, FMath::Min(PopAlpha / 0.68f, 1.0f), 2.5f);
		const float Overshoot = PopAlpha > 0.68f ? FMath::Sin((PopAlpha - 0.68f) / 0.32f * PI) * 0.16f : 0.0f;
		Scale *= Growth + Overshoot;
	}
	else
	{
		const float FadeAlpha = FMath::Clamp((Age - PopDuration) / (Lifetime - PopDuration), 0.0f, 1.0f);
		const float EasedFade = FMath::InterpEaseIn(0.0f, 1.0f, FadeAlpha, 1.7f);
		Scale *= FMath::Lerp(1.18f, 0.42f, EasedFade);
		Alpha = 1.0f - EasedFade;
		Color = FMath::Lerp(StartColor, FLinearColor::White, EasedFade);
	}

	SetActorLocation(SpawnLocation + Drift * Age + FVector(0.0f, 0.0f, FMath::Sin(Age * PI * 2.0f) * 5.0f));
	if (APlayerCameraManager* CameraManager = UGameplayStatics::GetPlayerCameraManager(this, 0))
	{
		const FRotator Facing = (CameraManager->GetCameraLocation() - GetActorLocation()).Rotation();
		SetActorRotation(FRotator(Facing.Pitch, Facing.Yaw, RollOffset));
	}
	SetActorScale3D(FVector(Scale));
	Color.A = Alpha;
	TextRender->SetTextRenderColor(Color.ToFColor(true));
}
