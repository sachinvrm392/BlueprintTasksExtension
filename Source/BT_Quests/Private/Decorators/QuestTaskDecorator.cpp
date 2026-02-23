// Copyright (C) Varian Daemon 2025. All Rights Reserved.


#include "Decorators/QuestTaskDecorator.h"

#include "Tasks/QuestTaskNode.h"
#include "DataAssets/QuestAsset.h"

#if WITH_EDITOR
TSharedRef<SWidget> UQuestTaskDecorator::CreateCenterContent(UClass* TaskClass,
                                                             UBtf_TaskForge* BlueprintTaskNode, UEdGraphNode* GraphNode)
{
	UQuestTaskNode* QuestTask = Cast<UQuestTaskNode>(BlueprintTaskNode);
	if(!QuestTask)
	{
		return Super::CreateCenterContent(TaskClass, BlueprintTaskNode, GraphNode);
	}

	/**Fetch the quest asset from the task. Will fail if we aren't using an instance.*/
	QuestInformation = QuestTask->QuestAsset.LoadSynchronous();

	if(!QuestInformation.IsValid())
	{
		/**Previous attempt of fetching a quest asset failed.
		 * Retrieve the QuestAsset pin and try to fetch its value
		 * and load the asset if it's found. */
		UEdGraphPin* QuestPin = GraphNode->FindPin(TEXT("QuestAsset"));
		if(QuestPin)
		{
			PRAGMA_DISABLE_DEPRECATION_WARNINGS
			TSoftObjectPtr<UQuestAsset> QuestAsset(QuestPin->DefaultValue);
			PRAGMA_ENABLE_DEPRECATION_WARNINGS
			QuestInformation = QuestAsset.LoadSynchronous();
		}
	}
	
	if(!QuestInformation.IsValid())
	{
		/**Both attempts failed*/
		return Super::CreateCenterContent(TaskClass, BlueprintTaskNode, GraphNode);
	}

	SAssignNew(CenterContentArea, SVerticalBox);
	
	//Title
	TSharedPtr<STextBlock> Title;
	CenterContentArea->AddSlot()
	.VAlign(VAlign_Top)
	.AutoHeight()
	.Padding(0, 10, 0, 0)
	[
	SAssignNew(Title, STextBlock)
		.Font(FCoreStyle::GetDefaultFontStyle("Bold", 18))
		.Text(QuestInformation->QuestName)
	];

	//Quest text. Make sure the text wraps and doesn't expand the node too much.
	CenterContentArea->AddSlot()
	.VAlign(VAlign_Top)
	.AutoHeight()
	[
		SNew(SBox)
		.VAlign(VAlign_Top)
		.MaxDesiredWidth(Title->GetDesiredSize().X)
		.Padding(0, 5)
		.Visibility(QuestInformation->QuestText.IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible)
		[
			SNew(STextBlock)
			.Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
			.AutoWrapText(true)
			.Text(QuestInformation->QuestText)
		]
	];

	TSharedPtr<SVerticalBox> TasksVerticalBox;
	CenterContentArea->AddSlot()
	// .AutoHeight()
	.VAlign(VAlign_Top)
	.AutoHeight()
	.Padding(0, 0, 0, 10)
	[
		SAssignNew(TasksVerticalBox, SVerticalBox)
	];
	//Create the task bullet points
	for(auto& CurrentGroup : QuestInformation->ObjectiveStages)
	{
		for(auto& CurrentObjective : CurrentGroup.Objectives)
		{
			FText TaskDisplayName;
			
			if(CurrentObjective.ObjectiveName.IsEmpty())
			{
				TaskDisplayName = FText::FromString("No task name!");
			}
			else
			{
				if(CurrentObjective.IsOptional)
				{
					TaskDisplayName = FText::FromString(FString::Printf(TEXT("%s / %f (Optional)"), *CurrentObjective.ObjectiveName.ToString(), CurrentObjective.ProgressRequired));
				}
				else
				{
					// TaskDisplayName = FText::FromString(FString::Printf(TEXT("%s / %d"), *CurrentObjective.ObjectiveName.ToString(), static_cast<int>(CurrentObjective.ProgressRequired)));
					TaskDisplayName = FText::FromString(FString::Printf(TEXT("%s / %.0f"), *CurrentObjective.ObjectiveName.ToString(), CurrentObjective.ProgressRequired));
				}
			}
		
		
			TasksVerticalBox->AddSlot()
			.Padding(0, 3)
			.AutoHeight()
			.VAlign(VAlign_Top)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.HAlign(HAlign_Left)
				.VAlign(VAlign_Center)
				.Padding(0, 0, 7, 0)
				[
					SNew(SImage)
					.Image(FAppStyle::GetBrush(TEXT("Icons.BulletPoint")))
					.DesiredSizeOverride(FVector2D(12.f, 12.f))
				]
				+ SHorizontalBox::Slot()
				[
					SNew(SBox)
					.MaxDesiredWidth(300)
					[
						SNew(STextBlock)
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
						.AutoWrapText(true)
						.Text(TaskDisplayName)
					]
				]
			];
		}
	}
	
	return CenterContentArea.ToSharedRef();	
}

#endif

TArray<UObject*> UQuestTaskDecorator::Get_ObjectsForExtraDetailsPanels() const
{
	TArray<UObject*> Objects;

	if(QuestInformation.IsValid())
	{
		Objects.Add(QuestInformation.Get());
	}
	
	return Objects;
}
