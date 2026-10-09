#pragma once

#include "Items/ChopItMandrakeScream.h"
#include "ChopItMandrakePreview.generated.h"

/** Placeable, harmless showcase for the mandrake mesh and Niagara effects. */
UCLASS(Blueprintable, meta=(DisplayName="Mandrágora FX Preview"))
class CHOPITCOMBAT_API AChopItMandrakePreview : public AChopItMandrakeScream
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
};
