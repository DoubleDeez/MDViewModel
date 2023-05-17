#pragma once

#include "CoreMinimal.h"
#include "FieldNotification/FieldMulticastDelegate.h"
#include "FieldNotification/FieldNotificationDeclaration.h"
#include "FieldNotification/IFieldValueChanged.h"
#include "UObject/Object.h"
#include "MDViewModelBase.generated.h"

struct FInstancedStruct;
class UUserWidget;
/**
 * Base UObject that adds FieldNotify support
 */
UCLASS(Abstract, BlueprintType)
class MDVIEWMODEL_API UMDViewModelBase : public UObject, public INotifyFieldValueChanged
{
	GENERATED_BODY()

public:
	// Called by view model providers after they create the view model object
	virtual void InitializeViewModel(const FInstancedStruct& ViewModelSettings) {};

	// Called by view model providers when they stop referencing the view model object
	virtual void ShutdownViewModel() {}

	// Override this to expose properties in the view model assignment editor, called on the CDO
	virtual UScriptStruct* GetViewModelSettingsStruct() const { return nullptr; }

	struct MDVIEWMODEL_API FFieldNotificationClassDescriptor : public ::UE::FieldNotification::IClassDescriptor
	{
	};

	virtual FDelegateHandle AddFieldValueChangedDelegate(UE::FieldNotification::FFieldId InFieldId, FFieldValueChangedDelegate InNewDelegate) override;
	virtual bool RemoveFieldValueChangedDelegate(UE::FieldNotification::FFieldId InFieldId, FDelegateHandle InHandle) override;
	virtual int32 RemoveAllFieldValueChangedDelegates(const void* InUserObject) override;
	virtual int32 RemoveAllFieldValueChangedDelegates(UE::FieldNotification::FFieldId InFieldId, const void* InUserObject) override;
	virtual const UE::FieldNotification::IClassDescriptor& GetFieldNotificationDescriptor() const override;

	// These feel like an anti-pattern but they're here for flexibility
	virtual void OnSetOnWidget(UUserWidget* Widget) {}
	virtual void OnUnsetFromWidget(UUserWidget* Widget) {}

	// Listen for changes to the specified field
	UFUNCTION(BlueprintCallable, Category = "FieldNotify", meta = (DisplayName = "Add Field Value Changed Delegate", ScriptName = "AddFieldValueChangedDelegate"))
	void K2_AddFieldValueChangedDelegate(FFieldNotificationId FieldId, FFieldValueChangedDynamicDelegate Delegate);

	// Stop listening for changes to the specified field
	UFUNCTION(BlueprintCallable, Category = "FieldNotify", meta = (DisplayName = "Remove Field Value Changed Delegate", ScriptName="RemoveFieldValueChangedDelegate"))
	void K2_RemoveFieldValueChangedDelegate(FFieldNotificationId FieldId, FFieldValueChangedDynamicDelegate Delegate);

protected:
	void BroadcastFieldValueChanged(UE::FieldNotification::FFieldId InFieldId);

	// Broadcast that the specified field has changed
	UFUNCTION(BlueprintCallable, Category="FieldNotify", meta = (DisplayName="Broadcast Field Value Changed", ScriptName="BroadcastFieldValueChanged"))
	void K2_BroadcastFieldValueChanged(FFieldNotificationId FieldId);

private:
	UE::FieldNotification::FFieldMulticastDelegate FieldNotifyDelegates;
	TBitArray<> EnabledFieldNotifications;
};
