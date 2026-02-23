// Copyright (C) Varian Daemon 2025. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "DataAssets/QuestAsset.h"
#include "Developer/I_AssetDetails.h"
#include "UObject/Object.h"
#include "QuestRequirementBase.generated.h"

class UQuestSystem;
/**
 * 
 */
UCLASS(Abstract, Blueprintable, BlueprintType, EditInlineNew, CollapseCategories,
	meta=(ContextMenuCategory = "Varian's Plugins", ContextMenuEntryName = "Quests|Quest Requirement", ContextMenuPrefix = "QR_"))
class BT_QUESTS_API UQuestRequirementBase : public UObject, public II_AssetDetails
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, BlueprintPure, BlueprintNativeEvent)
	bool IsConditionMet(const TSoftObjectPtr<UQuestAsset>& Quest);

	virtual UWorld* GetWorld() const override;

	virtual FLinearColor GetAssetColor_Implementation() const override
	{
		return OmniToolbox::BlueprintAssetColor;
	}

	virtual bool AppearsInContextMenu_Implementation() const override
	{
		return GetClass() == UQuestRequirementBase::StaticClass();
	}

	virtual TArray<FText> GetAssetsCategories_Implementation() const override
	{
		return { FText::FromString("Quest System") };
	}
};
