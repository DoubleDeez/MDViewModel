#include "ViewModelTab/MDViewModelAssignmentEditorObject.h"

#include "EdGraphSchema_K2.h"
#include "Engine/Blueprint.h"
#include "UObject/UObjectIterator.h"
#include "ViewModel/MDViewModelBase.h"
#include "ViewModelProviders/MDViewModelProviderBase.h"
#include "ViewModelProviders/MDViewModelProvider_Cached.h"


void UMDViewModelAssignmentEditorObject::PopulateFromAssignment(const FMDViewModelEditorAssignment& Assignment, UBlueprint* Blueprint)
{
	ViewModelClass = Assignment.Assignment.ViewModelClass;
	ViewModelProvider = Assignment.Assignment.ProviderTag;
	ViewModelInstanceName = Assignment.Assignment.ViewModelName;

	bOverrideName = (ViewModelInstanceName != MDViewModelUtils::DefaultViewModelName);

	ProviderSettings = Assignment.Data.ProviderSettings;

	if (const UMDViewModelProviderBase* Provider = MDViewModelUtils::FindViewModelProvider(ViewModelProvider))
	{
		if (ProviderSettings.GetScriptStruct() != Provider->GetProviderSettingsStruct())
		{
			ProviderSettings.InitializeAs(Provider->GetProviderSettingsStruct());
		}

		Provider->OnProviderSettingsInitializedInEditor(ProviderSettings, Blueprint, Assignment.Assignment);
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

	const FMDViewModelProvider_Cached_Settings* SettingsPtr = ProviderSettings.GetPtr<FMDViewModelProvider_Cached_Settings>();
	if (SettingsPtr != nullptr)
	{
		const UClass* RelativeObjectClass = (SettingsPtr->GetLifetimeTag() == TAG_MDVMProvider_Cached_Lifetimes_RelativeProperty) ? WidgetSkeletonClass.Get() : SettingsPtr->RelativeViewModel.ViewModelClass.LoadSynchronous();
		if (IsValid(RelativeObjectClass))
		{
			TSet<FName> FieldNotifySupportedNames;
			{
				const TScriptInterface<INotifyFieldValueChanged> DefaultWidget = RelativeObjectClass->GetDefaultObject();
				const UE::FieldNotification::IClassDescriptor& ClassDescriptor = DefaultWidget->GetFieldNotificationDescriptor();
				ClassDescriptor.ForEachField(RelativeObjectClass, [&FieldNotifySupportedNames](UE::FieldNotification::FFieldId FieldId)
				{
					FieldNotifySupportedNames.Add(FieldId.GetName());
					return true;
				});
			}

			for (TFieldIterator<FProperty> It(RelativeObjectClass); It; ++It)
			{
				if (const FObjectPropertyBase* ObjectProp = CastField<FObjectPropertyBase>(*It))
				{
					if (FieldNotifySupportedNames.Contains(ObjectProp->GetFName()))
					{
						Result.Add(ObjectProp->GetFName());
					}
				}
			}

			for (TFieldIterator<UFunction> It(RelativeObjectClass); It; ++It)
			{
				if (const UFunction* Func = *It)
				{
					if (FieldNotifySupportedNames.Contains(Func->GetFName()))
					{
						if (const FObjectPropertyBase* ReturnProperty = CastField<FObjectPropertyBase>(Func->GetReturnProperty()))
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
