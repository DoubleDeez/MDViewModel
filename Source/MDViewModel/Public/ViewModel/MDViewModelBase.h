#pragma once

#include "CoreMinimal.h"
#include "FieldNotification/FieldMulticastDelegate.h"
#include "FieldNotification/FieldNotificationDeclaration.h"
#include "FieldNotification/IFieldValueChanged.h"
#include "UObject/Object.h"
#include "MDViewModelBase.generated.h"

struct FInstancedStruct;
class UUserWidget;

#if WITH_EDITOR
class UWidgetBlueprint;
#endif

// Fix syntax highlighting caused by code analysis engine not detecting FieldNotify names
#if defined(__RESHARPER__)
// Broadcast that the specified field has changed
#define MDVM_BROADCAST_FIELD(FIELD_NAME) [_ = FIELD_NAME](){}()
#else
// Broadcast that the specified field has changed
#define MDVM_BROADCAST_FIELD(FIELD_NAME) BroadcastFieldValueChanged(ThisClass::FFieldNotificationClassDescriptor::FIELD_NAME)
#endif

// Fix syntax highlighting caused by code analysis engine not detecting FieldNotify names, attempts to maintain traits of SetFieldNotifyValue
#if defined(__RESHARPER__)
// Check if the field value is different than the passed in value then set it and broadcast if it changed.
#define MDVM_SET_FIELD(FIELD_NAME, VALUE) [_ = FIELD_NAME, __ = VALUE](){ return FMath::RandBool(); }()
#else
// Check if the field value is different than the passed in value then set it and broadcast if it changed.
#define MDVM_SET_FIELD(FIELD_NAME, VALUE) SetFieldNotifyValue(FIELD_NAME, VALUE, ThisClass::FFieldNotificationClassDescriptor::FIELD_NAME)
#endif

/**
 * Base UObject that adds FieldNotify support.
 * Should not have an outer object (other than the transient package), relies on ContextObject for GetWorld().
 *
 * InitializeViewModelWithContext() and ShutdownViewModel() are expected to be called by the creator of this object.
 */
UCLASS(Abstract, BlueprintType, Within=Package)
class MDVIEWMODEL_API UMDViewModelBase : public UObject, public INotifyFieldValueChanged
{
	GENERATED_BODY()

public:
	// Called by view model providers after they create the view model object
	void InitializeViewModelWithContext(const FInstancedStruct& ViewModelSettings, UObject* InContextObject);

	// Called by view model providers when they stop referencing the view model object
	void ShutdownViewModelFromProvider();

	virtual UWorld* GetWorld() const override;

#if WITH_EDITOR
	// Override this to expose properties in the view model assignment editor, called on the CDO
	virtual UScriptStruct* GetViewModelSettingsStruct() const { return nullptr; }
	virtual bool ValidateViewModelSettings(const FInstancedStruct& Settings, UWidgetBlueprint* WidgetBlueprint, TArray<FText>& OutIssues) const { return true; }
#endif
	
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

	template<typename T = UObject>
	T* GetContextObject() const
	{
		if (ContextObject.IsValid())
		{
			return Cast<T>(ContextObject.Get());
		}

		return Cast<T>(GetOuter());
	}

	FSimpleMulticastDelegate OnViewModelShutDown;

protected:
	// Called by view model providers after they create the view model object
	virtual void InitializeViewModel(const FInstancedStruct& ViewModelSettings) {};

	// Called by view model providers when they stop referencing the view model object
	virtual void ShutdownViewModel() {}
	
	void BroadcastFieldValueChanged(UE::FieldNotification::FFieldId InFieldId);

	// Helper that changes for equality before setting and broadcasting the specified field. Uses operator==.
	// Returns whether the field value was changed or not
	template<typename T, typename U>
	bool SetFieldNotifyValue(T& Value, const U& NewValue, UE::FieldNotification::FFieldId FieldId)
	{
		if (Value == NewValue)
		{
			return false;
		}

		Value = NewValue;
		BroadcastFieldValueChanged(FieldId);
		return true;
	}

	// Broadcast that the specified field has changed
	UFUNCTION(BlueprintCallable, Category="FieldNotify", meta = (DisplayName="Broadcast Field Value Changed", ScriptName="BroadcastFieldValueChanged"))
	void K2_BroadcastFieldValueChanged(FFieldNotificationId FieldId);

private:
	UPROPERTY(Transient)
	TWeakObjectPtr<UObject> ContextObject;
	
	UE::FieldNotification::FFieldMulticastDelegate FieldNotifyDelegates;
	TBitArray<> EnabledFieldNotifications;
};
