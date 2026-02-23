// Copyright (C) Varian Daemon 2025. All Rights Reserved.


#include "DataAssets/QuestChain.h"
#include "DataAssets/QuestAsset.h"
#include "UObject/AssetRegistryTagsContext.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

void UQuestChain::GetAssetRegistryTags(FAssetRegistryTagsContext Context) const
{
	Context.AddTag(FAssetRegistryTag(BTE::QuestChainName_Tag, ChainName.ToString(), FAssetRegistryTag::TT_Alphabetical));
	
	Super::GetAssetRegistryTags(Context);
}

#if WITH_EDITOR
EDataValidationResult UQuestChain::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = CombineDataValidationResults(Super::IsDataValid(Context), EDataValidationResult::Valid);

	if(Stages.IsEmpty())
	{
		Result = EDataValidationResult::Invalid;
		Context.AddError(FText::FromString("Stages is empty"));
	}
	else
	{
		for(auto& CurrentStage : Stages)
		{
			if(CurrentStage.Quests.IsEmpty())
			{
				Result = EDataValidationResult::Invalid;
				Context.AddError(FText::FromString("Stage has no quests"));
			}

			for(auto& CurrentQuest : CurrentStage.Quests)
			{
				if(CurrentQuest.IsNull())
				{
					continue;
				}
				
				if(!CurrentQuest->QuestChains.Contains(this))
				{
					Result = EDataValidationResult::Invalid;
					Context.AddError(FText::FromString(FString::Printf(TEXT("%s does not link back to this chain via its QuestChains array"), *CurrentQuest.GetAssetName())));
				}
			}
		}
	}
	
	return Result;
}

void UQuestChain::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	for(auto& CurrentStage : Stages)
	{
		for(auto& CurrentQuest : CurrentStage.Quests)
		{
			if(CurrentQuest.IsNull())
			{
				continue;
			}

			CurrentQuest.LoadSynchronous()->QuestChains.AddUnique(this);
		}
	}
}
#endif