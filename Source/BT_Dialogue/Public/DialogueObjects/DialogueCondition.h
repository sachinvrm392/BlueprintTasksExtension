// Copyright (C) Varian Daemon 2025. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Developer/I_AssetDetails.h"
#include "UObject/Object.h"
#include "DialogueCondition.generated.h"

/**
 * 
 */
UCLASS(Abstract, Blueprintable, BlueprintType, AutoExpandCategories = ("Default"), EditInlineNew)
class BT_DIALOGUE_API UDialogueCondition : public UObject, public II_AssetDetails
{
	GENERATED_BODY()

public:

	UFUNCTION(Category = "Dialogue", BlueprintCallable, BlueprintPure, BlueprintNativeEvent)
	bool IsConditionMet();

	virtual UWorld* GetWorld() const override;
	
	virtual FLinearColor GetAssetColor_Implementation() const override
	{
		return OmniToolbox::BlueprintAssetColor;
	}

	virtual bool AppearsInContextMenu_Implementation() const override
	{
		return GetClass() == UDialogueCondition::StaticClass();
	}

	virtual TArray<FText> GetAssetsCategories_Implementation() const override
	{
		return { FText::FromString("Dialogue System") };
	}
};
