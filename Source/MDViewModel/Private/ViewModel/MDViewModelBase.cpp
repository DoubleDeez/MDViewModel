#include "ViewModel/MDViewModelBase.h"

void UMDViewModelBase::InitializeViewModelWithContext(const FInstancedStruct& ViewModelSettings, UObject* InContextObject)
{
	ContextObject = InContextObject;
	InitializeViewModel(ViewModelSettings);
}

void UMDViewModelBase::InitializeViewModelWithContext(const FInstancedStruct& ViewModelSettings, const UObject* InContextObject)
{
	// TODO - Determine a way to support both const and non-const context objects
	InitializeViewModelWithContext(ViewModelSettings, const_cast<UObject*>(InContextObject));
}

void UMDViewModelBase::ShutdownViewModelFromProvider()
{
	ShutdownViewModel();
	
	OnViewModelShutDown.Broadcast();
}

UWorld* UMDViewModelBase::GetWorld() const
{
	if (const UObject* Context = GetContextObject<UObject>())
	{
		return Context->GetWorld();
	}
	
	return nullptr;
}

FDelegateHandle UMDViewModelBase::AddFieldValueChangedDelegate(UE::FieldNotification::FFieldId InFieldId, FFieldValueChangedDelegate InNewDelegate)
{
	FDelegateHandle Result;
	if (InFieldId.IsValid())
	{
		Result = FieldNotifyDelegates.Add(this, InFieldId, MoveTemp(InNewDelegate));
		if (Result.IsValid())
		{
			EnabledFieldNotifications.PadToNum(InFieldId.GetIndex() + 1, false);
			EnabledFieldNotifications[InFieldId.GetIndex()] = true;
		}
	}
	return Result;
}

bool UMDViewModelBase::RemoveFieldValueChangedDelegate(UE::FieldNotification::FFieldId InFieldId, FDelegateHandle InHandle)
{
	bool bResult = false;
	if (InFieldId.IsValid() && InHandle.IsValid() && EnabledFieldNotifications.IsValidIndex(InFieldId.GetIndex()) && EnabledFieldNotifications[InFieldId.GetIndex()])
	{
		const UE::FieldNotification::FFieldMulticastDelegate::FRemoveFromResult RemoveResult = FieldNotifyDelegates.RemoveFrom(this, InFieldId, InHandle);
		bResult = RemoveResult.bRemoved;
		EnabledFieldNotifications[InFieldId.GetIndex()] = RemoveResult.bHasOtherBoundDelegates;
	}
	return bResult;
}

int32 UMDViewModelBase::RemoveAllFieldValueChangedDelegates(const void* InUserObject)
{
	int32 bResult = 0;
	if (InUserObject)
	{
		const UE::FieldNotification::FFieldMulticastDelegate::FRemoveAllResult RemoveResult = FieldNotifyDelegates.RemoveAll(this, InUserObject);
		bResult = RemoveResult.RemoveCount;
		EnabledFieldNotifications = RemoveResult.HasFields;
	}
	return bResult;
}

int32 UMDViewModelBase::RemoveAllFieldValueChangedDelegates(UE::FieldNotification::FFieldId InFieldId, const void* InUserObject)
{
	int32 bResult = 0;
	if (InUserObject)
	{
		const UE::FieldNotification::FFieldMulticastDelegate::FRemoveAllResult RemoveResult = FieldNotifyDelegates.RemoveAll(this, InFieldId, InUserObject);
		bResult = RemoveResult.RemoveCount;
		EnabledFieldNotifications = RemoveResult.HasFields;
	}
	return bResult;
}

const UE::FieldNotification::IClassDescriptor& UMDViewModelBase::GetFieldNotificationDescriptor() const
{
	static FFieldNotificationClassDescriptor Local;
	return Local;
}

void UMDViewModelBase::K2_AddFieldValueChangedDelegate(FFieldNotificationId InFieldId, FFieldValueChangedDynamicDelegate InDelegate)
{
	if (InFieldId.IsValid())
	{
		const UE::FieldNotification::FFieldId FieldId = GetFieldNotificationDescriptor().GetField(GetClass(), InFieldId.FieldName);
		if (ensureMsgf(FieldId.IsValid(), TEXT("The field should be compiled correctly.")))
		{
			if (FieldNotifyDelegates.Add(this, FieldId, InDelegate).IsValid())
			{
				EnabledFieldNotifications.PadToNum(FieldId.GetIndex() + 1, false);
				EnabledFieldNotifications[FieldId.GetIndex()] = true;
			}
		}
	}
}

void UMDViewModelBase::K2_RemoveFieldValueChangedDelegate(FFieldNotificationId InFieldId, FFieldValueChangedDynamicDelegate InDelegate)
{
	if (InFieldId.IsValid())
	{
		const UE::FieldNotification::FFieldId FieldId = GetFieldNotificationDescriptor().GetField(GetClass(), InFieldId.FieldName);
		if (ensureMsgf(FieldId.IsValid(), TEXT("The field should be compiled correctly.")))
		{
			if (EnabledFieldNotifications.IsValidIndex(FieldId.GetIndex()) && EnabledFieldNotifications[FieldId.GetIndex()])
			{
				const UE::FieldNotification::FFieldMulticastDelegate::FRemoveFromResult RemoveResult = FieldNotifyDelegates.RemoveFrom(this, FieldId, InDelegate);
				EnabledFieldNotifications[FieldId.GetIndex()] = RemoveResult.bHasOtherBoundDelegates;
			}
		}
	}
}

UObject* UMDViewModelBase::GetContextObject() const
{
	return GetContextObject<UObject>();
}

void UMDViewModelBase::BroadcastFieldValueChanged(UE::FieldNotification::FFieldId InFieldId)
{
	if (InFieldId.IsValid() && EnabledFieldNotifications.IsValidIndex(InFieldId.GetIndex()) && EnabledFieldNotifications[InFieldId.GetIndex()])
	{
		FieldNotifyDelegates.Broadcast(this, InFieldId);
	}
}

bool UMDViewModelBase::SetFieldNotifyValue(FText& Value, const FText& NewValue, UE::FieldNotification::FFieldId FieldId)
{
	if (Value.EqualTo(NewValue))
	{
		return false;
	}

	Value = NewValue;
	BroadcastFieldValueChanged(FieldId);
	return true;
}

void UMDViewModelBase::K2_BroadcastFieldValueChanged(FFieldNotificationId InFieldId)
{
	if (InFieldId.IsValid())
	{
		const UE::FieldNotification::FFieldId FieldId = GetFieldNotificationDescriptor().GetField(GetClass(), InFieldId.FieldName);
		if (ensureMsgf(FieldId.IsValid(), TEXT("The field should be compiled correctly.")))
		{
			BroadcastFieldValueChanged(FieldId);
		}
	}
}
