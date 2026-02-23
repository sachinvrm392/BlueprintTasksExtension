// Copyright (C) Varian Daemon 2025. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Developer/I_AssetDetails.h"
#include "Engine/DataAsset.h"
#include "QuestChain.generated.h"

class UQuestAsset;

USTRUCT(Blueprintable)
struct FBTQuestChainStage
{
	GENERATED_BODY()

	UPROPERTY(Category = "Quest Chain stage", BlueprintReadWrite, EditAnywhere)
	TArray<TSoftObjectPtr<UQuestAsset>> Quests;
};

/**
 * An asset that manages a set of quests, creating a "quest chain"
 */
UCLASS()
class BT_QUESTS_API UQuestChain : public UPrimaryDataAsset, public II_AssetDetails
{
	GENERATED_BODY()

public:

	UPROPERTY(Category = "Quest Chain", BlueprintReadOnly, EditAnywhere)
	FText ChainName;
	
	UPROPERTY(Category = "Quest Chain", BlueprintReadOnly, EditAnywhere)
	TArray<FBTQuestChainStage> Stages;
	
	virtual void GetAssetRegistryTags(FAssetRegistryTagsContext Context) const override;

#if WITH_EDITOR

	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;

	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	
#endif

	virtual bool AppearsInContextMenu_Implementation() const override
	{
		return GetClass() == UQuestChain::StaticClass();
	}

	virtual TArray<FText> GetAssetsCategories_Implementation() const override
	{
		return { FText::FromString("Quest System") };
	}
};
