// Copyright (C) Varian Daemon 2025. All Rights Reserved.


#include "DataAssets/QuestAsset.h"

#include "UObject/AssetRegistryTagsContext.h"

FPrimaryAssetId UQuestAsset::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(FName("Quest Asset"), GetFName());
}

void UQuestAsset::GetAssetRegistryTags(FAssetRegistryTagsContext Context) const
{
	Context.AddTag(FAssetRegistryTag(BTE::QuestID_Tag, QuestID.ToString(), FAssetRegistryTag::TT_Alphabetical));
	Context.AddTag(FAssetRegistryTag(BTE::QuestName_Tag, QuestName.ToString(), FAssetRegistryTag::TT_Alphabetical));
	
	Super::GetAssetRegistryTags(Context);
}
