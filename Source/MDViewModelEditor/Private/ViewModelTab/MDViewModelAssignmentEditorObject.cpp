#include "ViewModelTab/MDViewModelAssignmentEditorObject.h"

#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "MDViewModelModule.h"
#include "UObject/UObjectIterator.h"
#include "ViewModel/MDViewModelBase.h"
#include "ViewModelProviders/MDViewModelProviderBase.h"
#include "WidgetBlueprint.h"


void UMDViewModelAssignmentEditorObject::PopulateFromAssignment(const FMDViewModelEditorAssignment& Assignment, UWidgetBlueprint* WidgetBlueprint)
{
	ViewModelClass = Assignment.Assignment.ViewModelClass;
	ViewModelProvider = Assignment.Assignment.ProviderTag;
	ViewModelInstanceName = Assignment.Assignment.ViewModelName;

	bOverrideName = (ViewModelInstanceName != MDViewModelUtils::DefaultViewModelName);

	ProviderSettings = Assignment.Data.ProviderSettings;

	if (UMDViewModelProviderBase* Provider = MDViewModelUtils::FindViewModelProvider(ViewModelProvider))
	{
		if (ProviderSettings.GetScriptStruct() != Provider->GetProviderSettingsStruct())
		{
			ProviderSettings.InitializeAs(Provider->GetProviderSettingsStruct());
		}

		Provider->OnProviderSettingsInitializedInEditor(ProviderSettings, WidgetBlueprint);
	}
	else
	{
		ProviderSettings.Reset();
	}

	const UMDViewModelBase* ViewModelCDO = ViewModelClass->GetDefaultObject<UMDViewModelBase>();
	if (Assignment.Data.ViewModelSettings.IsValid() && Assignment.Data.ViewModelSettings.GetScriptStruct() == ViewModelCDO->GetViewModelSettingsStruct())
	{
		ViewModelSettings = Assignment.Data.ViewModelSettings;
	}
	else
	{
		ViewModelSettings.InitializeAs(ViewModelCDO->GetViewModelSettingsStruct());
	}
}

FMDViewModelEditorAssignment UMDViewModelAssignmentEditorObject::CreateAssignment() const
{
	FMDViewModelEditorAssignment Assignment;

	Assignment.Assignment.ViewModelClass = ViewModelClass;
	Assignment.Assignment.ProviderTag = ViewModelProvider;
	Assignment.Assignment.ViewModelName = bOverrideName ? ViewModelInstanceName : MDViewModelUtils::DefaultViewModelName;

	Assignment.Data.ProviderSettings = ProviderSettings;
	Assignment.Data.ViewModelSettings = ViewModelSettings;

	return Assignment;
}

TArray<FName> UMDViewModelAssignmentEditorObject::GetRelativePropertyNames() const
{
	TArray<FName> Result;

	Result.Add(NAME_None);

	if (IsValid(WidgetSkeletonClass))
	{
		TSet<FName> FieldNotifySupportedNames;
		{
			const TScriptInterface<INotifyFieldValueChanged> DefaultWidget = WidgetSkeletonClass->GetDefaultObject();
			const UE::FieldNotification::IClassDescriptor& ClassDescriptor = DefaultWidget->GetFieldNotificationDescriptor();
			ClassDescriptor.ForEachField(WidgetSkeletonClass, [&FieldNotifySupportedNames](UE::FieldNotification::FFieldId FieldId)
			{
				FieldNotifySupportedNames.Add(FieldId.GetName());
				return true;
			});
		}

		for (TFieldIterator<FProperty> It(WidgetSkeletonClass); It; ++It)
		{
			if (const FObjectPropertyBase* ObjectProp = CastField<FObjectPropertyBase>(*It))
			{
				if (FieldNotifySupportedNames.Contains(ObjectProp->GetFName()))
				{
					const bool bIsSupportedObjectType =
						ObjectProp->PropertyClass->IsChildOf<AActor>()
						|| ObjectProp->PropertyClass->IsChildOf<UWorld>()
						|| ObjectProp->PropertyClass->IsChildOf<ULocalPlayer>()
						|| ObjectProp->PropertyClass->IsChildOf<UGameInstance>();
					if (bIsSupportedObjectType)
					{
						Result.Add(ObjectProp->GetFName());
					}
				}
			}
		}

		for (TFieldIterator<UFunction> It(WidgetSkeletonClass); It; ++It)
		{
			if (const UFunction* Func = *It)
			{
				if (FieldNotifySupportedNames.Contains(Func->GetFName()))
				{
					if (const FObjectPropertyBase* ReturnProperty = CastField<FObjectPropertyBase>(Func->GetReturnProperty()))
					{
						const bool bIsSupportedObjectType =
							ReturnProperty->PropertyClass->IsChildOf<AActor>()
							|| ReturnProperty->PropertyClass->IsChildOf<UWorld>()
							|| ReturnProperty->PropertyClass->IsChildOf<ULocalPlayer>()
							|| ReturnProperty->PropertyClass->IsChildOf<UGameInstance>();
						if (bIsSupportedObjectType)
						{
							Result.Add(ReturnProperty->GetFName());
						}
					}
				}
			}
		}
	}

	Result.Sort([](const FName& A, const FName& B)
	{
		if (A == NAME_None || B == NAME_None)
		{
			return A == NAME_None;
		}

		return A.Compare(B) < 0;
	});

	return Result;
}
