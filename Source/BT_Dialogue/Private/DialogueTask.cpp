// Copyright (C) Varian Daemon 2025. All Rights Reserved.


#include "DialogueTask.h"

#include "DialogueTaskDecorator.h"
#if TAGFACTS_INSTALLED
#include "Core/FactSubSystem.h"
#endif
#include "DataAssets/DialogueCharacter.h"

UDialogueTask::UDialogueTask(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
#if WITH_EDITORONLY_DATA
	Category = "Dialogue";
	MenuDisplayName = "Dialogue Task";
	Tooltip = "A task that displays a dialogue box.";
	
	Decorator = UDialogueTaskDecorator::StaticClass();
	
	if(HasAnyFlags(RF_ClassDefaultObject))
	{
		ExecFunction.Empty();
	}
#endif
}

bool UDialogueTask::Get_NodeTitleColor_Implementation(FLinearColor& Color)
{
	Color = FLinearColor::Green;
	return true;
}

UTexture2D* UDialogueTask::GetSpeakerPortrait()
{
	if(Script.Character.IsNull())
	{
		return nullptr;
	}

	if(Script.Character.LoadSynchronous()->CharacterPortrait.IsNull())
	{
		return nullptr;
	}
	
	return Cast<UTexture2D>(Script.Character->CharacterPortrait.LoadSynchronous());
}

TArray<FCustomOutputPin> UDialogueTask::Get_CustomOutputPins_Implementation() const
{
	TArray<FCustomOutputPin> Output;

	if(Script.NotInteractive)
	{
		return Output;
	}

	for(auto& DialogueOption : Script.DialogueOptions)
	{
		FString ToolTipText;
		for(int32 i = 0; i < DialogueOption.DialogueTexts.Num(); i++)
		{
			ToolTipText += "- " + DialogueOption.DialogueTexts[i].DialogueText.ToString();
			if(DialogueOption.DialogueTexts.IsValidIndex(i + 1))
			{
				ToolTipText += LINE_TERMINATOR;
				ToolTipText += LINE_TERMINATOR;
			}
		}
		
		Output.Add(FCustomOutputPin(DialogueOption.ButtonText.ToString(), ToolTipText));
	}
	
	return Output;
}

void UDialogueTask::SelectDialogueOption(FDialogueTaskOption Option)
{
	#if TAGFACTS_INSTALLED
	if(Option.MemorySettings.RememberOptionSelection)
	{
		UFactSubSystem* FactSubSystem = UFactSubSystem::Get();
		FactSubSystem->IncrementFact(Option.MemorySettings.OptionFact, 1);
	}
	#endif

	TriggerCustomOutputPin(FName(Option.ButtonText.ToString()), TInstancedStruct<FCustomOutputPinData>::Make<FDialogueTaskOption>(Option));
}

TArray<FString> UDialogueTask::ValidateNodeDuringCompilation_Implementation()
{
	TArray<FString> Errors;

	if(Script.Character.IsNull())
	{
		Errors.Add(FString::Printf(TEXT("Dialogue node does not reference a character")));
	}

	if(Script.CharacterDialogue.IsEmpty())
	{
		Errors.Add(FString::Printf(TEXT("Dialogue node does not contain character dialogue")));
	}
	else
	{
		for(int32 i = 0; i < Script.CharacterDialogue.Num(); i++)
		{
			if(Script.CharacterDialogue[i].DialogueText.IsEmpty())
			{
				Errors.Add(FString::Printf(TEXT("Character Dialogue index %i does not contain text"), i));
			}
		}
	}

	if(!Script.NotInteractive)
	{
		if(Script.DialogueOptions.IsEmpty())
		{
			Errors.Add(FString::Printf(TEXT("Dialogue is set to be interactive, but has no dialogue options")));
		}
		for(int32 i = 0; i < Script.DialogueOptions.Num(); i++)
		{
			if(Script.DialogueOptions[i].ButtonText.IsEmpty())
			{
				Errors.Add(FString::Printf(TEXT("Dialogue index %i does not contain button text"), i));
			}

			if(Script.DialogueOptions[i].DialogueTexts.IsEmpty())
			{
				Errors.Add(FString::Printf(TEXT("Dialogue index %i does not contain any dialogue options"), i));
			}

			for(int32 DialogueTextIndex = 0; DialogueTextIndex < Script.DialogueOptions[i].DialogueTexts.Num(); DialogueTextIndex++)
			{
				if(Script.DialogueOptions[i].DialogueTexts[DialogueTextIndex].DialogueText.IsEmpty())
				{
					Errors.Add(FString::Printf(TEXT("Dialogue index %i, option %i does not contain dialogue text"), i, DialogueTextIndex));
				}
			}

			for(auto& CurrentCondition : Script.DialogueOptions[i].ConditionSettings.Conditions)
			{
				if(!CurrentCondition)
				{
					Errors.Add(FString::Printf(TEXT("Dialogue index %i contains a null condition"), i));
				}
			}

			if(Script.DialogueOptions[i].MemorySettings.RememberOptionSelection)
			{
				if(!Script.DialogueOptions[i].MemorySettings.OptionFact.IsValid())
				{
					Errors.Add(FString::Printf(TEXT("Dialogue node is set to be remembered, but has no tag assigned")));
				}
			}
		}
	}
	
	return Errors;
}

FString UDialogueTask::GetCenterText_Implementation()
{
	FString FinalText;
	for(int32 i = 0; i < Script.CharacterDialogue.Num(); i++)
	{
		FinalText += "- " + Script.CharacterDialogue[i].DialogueText.ToString();
		/**Check if this is the last index. If it isn't, add two line breaks */
		if(Script.CharacterDialogue.IsValidIndex(i + 1))
		{
			FinalText += LINE_TERMINATOR;
			FinalText += LINE_TERMINATOR;
		}
	}
	return FinalText;
}

