#pragma once

#include "Runtime/Launch/Resources/Version.h"

#if ENGINE_MAJOR_VERSION > 5 || ENGINE_MINOR_VERSION >= 3
#include "FieldNotificationDeclaration.h"
#include "FieldNotificationDelegate.h"
#include "INotifyFieldValueChanged.h"
#else
#include "FieldNotification/FieldMulticastDelegate.h"
#include "FieldNotification/FieldNotificationDeclaration.h"
#include "FieldNotification/IFieldValueChanged.h"
#endif

#include "InstancedStruct.h"
#include "UObject/Object.h"
#include "UObject/Package.h"
#include "Util/MDViewModelMetaUtils.h"
#include "MDViewModelBase.generated.h"

struct FMDViewModelAssignment;

#if WITH_EDITOR
class UBlueprint;
#endif

/**
 * Base UObject that adds FieldNotify support.
 * Should not have an outer object (other than the transient package), relies on ContextObject for GetWorld().
 *
 * InitializeViewModelWithContext() and ShutdownViewModel() are expected to be called by the creator of this object.
 */
UCLASS(Abstract, BlueprintType, Config=ViewModel, Within=Package)
class MDVIEWMODEL_API UMDViewModelBase : public UObject, public INotifyFieldValueChanged
{
	GENERATED_BODY()

public:
	virtual void BeginDestroy() override;

	// Called by view model providers after they create the view model object
	void InitializeViewModelWithContext(const FInstancedStruct& ViewModelSettings, UObject* InContextObject, const UObject* WorldContext = nullptr);
	void InitializeViewModelWithContext(const FInstancedStruct& ViewModelSettings, const UObject* InContextObject, const UObject* WorldContext = nullptr);
	void InitializeViewModelWithContext(UObject* InContextObject, const UObject* WorldContext = nullptr);
	void InitializeViewModelWithContext(const UObject* InContextObject, const UObject* WorldContext = nullptr);

	// Called by view model providers/owners when they stop referencing the view model object
	UFUNCTION(BlueprintCallable, Category = "View Model")
	void ShutdownViewModelFromProvider();

	virtual UWorld* GetWorld() const override;

	const UObject* GetWorldContextObject() const { return WorldContextObjectPtr.Get(); }

#if WITH_EDITOR
	// Override this to expose properties in the view model assignment editor, called on the CDO
	virtual UScriptStruct* GetViewModelSettingsStruct() const { return nullptr; }
	// Gives a place to massage the properties in ViewModelSettings when the user changes them
	virtual void OnViewModelSettingsPropertyChanged(FInstancedStruct& ViewModelSettings, UBlueprint* Blueprint, const FMDViewModelAssignment& Assignment) const {};
	// If this view model requires anything from ViewModelSettings or elsewhere, this can be overridden to message any issues to the user
	virtual bool ValidateViewModelSettings(const FInstancedStruct& ViewModelSettings, UBlueprint* Blueprint, const FMDViewModelAssignment& Assignment, TArray<FText>& OutIssues) const { return true; }
	// If this view model can only be initialized with specific context object types, this can be overridden to message those types to the user. The supported types is allowed to change based on the ViewModelSettings or Blueprint. Add nullptr to indicate that the context object is not used for this view model.
	virtual void GetSupportedContextObjectTypes(const FInstancedStruct& ViewModelSettings, UBlueprint* Blueprint, TArray<TSubclassOf<UObject>>& OutClasses) const {};
	// If the types gathered in GetSupportedContextObjectTypes differs from the type of the ContextObject being stored (due to RedirectContextObject), overridden this to report the types that will be stored
	virtual void GetStoredContextObjectTypes(const FInstancedStruct& ViewModelSettings, UBlueprint* Blueprint, TArray<TSubclassOf<UObject>>& OutClasses) const { GetSupportedContextObjectTypes(ViewModelSettings, Blueprint, OutClasses); };
#endif

	struct MDVIEWMODEL_API FFieldNotificationClassDescriptor : public ::UE::FieldNotification::IClassDescriptor
	{
	};

	virtual FDelegateHandle AddFieldValueChangedDelegate(UE::FieldNotification::FFieldId InFieldId, FFieldValueChangedDelegate InNewDelegate) override;
	virtual bool RemoveFieldValueChangedDelegate(UE::FieldNotification::FFieldId InFieldId, FDelegateHandle InHandle) override;
	virtual int32 RemoveAllFieldValueChangedDelegates(const void* InUserObject) override;
	virtual int32 RemoveAllFieldValueChangedDelegates(UE::FieldNotification::FFieldId InFieldId, const void* InUserObject) override;
	virtual const UE::FieldNotification::IClassDescriptor& GetFieldNotificationDescriptor() const override;

	template<typename T, typename U>
	FDelegateHandle AddTypedFieldValueChangedDelegate(UE::FieldNotification::FFieldId InFieldId, TDelegate<void(U)>&& Delegate);

	template<typename T>
	bool GetFieldValue(UE::FieldNotification::FFieldId FieldId, T& OutValue);

	// These feel like an anti-pattern but they're here for flexibility
	virtual void OnSetOnObject(UObject* Object) {}
	virtual void OnUnsetFromObject(UObject* Object) {}

	// Called on the CDO. Override to redirect the cache context object from the one from the selected Provider to one appropriate for your view model.
	// This can also affect the view model instance's context object. Can be used to move the responsibility of selecting a cache context from the editor to the view model implementer.
	// Any cache binding for the original context will still take place so the view models are updated appropriately (eg. Binding to the Owning Player but redirecting to a component on the player will still update the view model when the Owning Player changes)
	// ViewModelSettings are not guaranteed to be valid, it will depend on the source of the request for the cached view model
	virtual UObject* CDORedirectCachedContextObject(const UObject* WorldContextObject, UObject* ProvidedCacheContextObject, const FInstancedStruct& ViewModelSettings) const { return ProvidedCacheContextObject; }

	// Listen for changes to the specified field
	UFUNCTION(BlueprintCallable, Category = "FieldNotify", meta = (DisplayName = "Add Field Value Changed Delegate", ScriptName = "AddFieldValueChangedDelegate"))
	void K2_AddFieldValueChangedDelegate(FFieldNotificationId FieldId, FFieldValueChangedDynamicDelegate Delegate);

	// Stop listening for changes to the specified field
	UFUNCTION(BlueprintCallable, Category = "FieldNotify", meta = (DisplayName = "Remove Field Value Changed Delegate", ScriptName="RemoveFieldValueChangedDelegate"))
	void K2_RemoveFieldValueChangedDelegate(FFieldNotificationId FieldId, FFieldValueChangedDynamicDelegate Delegate);

	UFUNCTION(BlueprintCallable, Category = "View Model", meta = (DisplayName = "Get View Model Context Object"))
	UObject* GetContextObject() const;

	template<typename T>
	T* GetContextObject() const
	{
		return Cast<T>(ContextObject.Get());
	}

	template<typename T>
	T* GetContextObjectChecked() const
	{
		return CastChecked<T>(ContextObject.Get());
	}

	template<typename T>
	T* GetContextObjectEnsure() const
	{
		using DecayedT = typename TDecay<T>::Type;

		auto IsValidObject = [](const auto* Object)
		{
			if constexpr (TIsDerivedFrom<DecayedT, UObject>::Value)
			{
				return IsValid(Object);
			}

			if constexpr (TIsIInterface<DecayedT>::Value)
			{
				return IsValid(Cast<UObject>(Object));
			}

			return Object != nullptr;
		};

		T* Object = Cast<T>(ContextObject.Get());
		const bool bIsValid = ensureAlwaysMsgf(IsValidObject(Object), TEXT("The context object [%s] for view model [%s] is expected to be of type [%s]"), *GetFullNameSafe(ContextObject.Get()), *GetName(), *GetTypeName<DecayedT>());
		return bIsValid ? Object : nullptr;
	}


	UFUNCTION(BlueprintCallable, Category = "View Model")
	const FInstancedStruct& GetViewModelSettings() const { return CachedViewModelSettings; }

	template<typename T>
	const T* GetViewModelSettings() const
	{
		return GetViewModelSettings().GetPtr<T>();
	}

	template<typename T>
	const T& GetViewModelSettingsChecked() const
	{
		const FInstancedStruct& Settings = GetViewModelSettings();
		const T* SettingsPtr = Settings.GetPtr<T>();
		checkf(SettingsPtr != nullptr, TEXT("The view model [%s] is expecting settings of type [%s] but the actual type is [%s]"), *GetName(), *T::StaticStruct()->GetName(), *GetNameSafe(Settings.GetScriptStruct()));
		return *SettingsPtr;
	}

	template<typename T>
	const T* GetViewModelSettingsEnsure() const
	{
		const FInstancedStruct& Settings = GetViewModelSettings();
		const T* SettingsPtr = Settings.GetPtr<T>();
		ensureAlwaysMsgf(SettingsPtr != nullptr, TEXT("The view model [%s] is expecting settings of type [%s] but the actual type is [%s]"), *GetName(), *T::StaticStruct()->GetName(), *GetNameSafe(Settings.GetScriptStruct()));
		return SettingsPtr;
	}

	FSimpleMulticastDelegate OnViewModelShutDown;

protected:
	// Called by view model providers after they create the view model object
	virtual void InitializeViewModel() {};

	// Override this to override the context object that's been passed to this view model during initialization
	virtual UObject* RedirectContextObject(UObject* InContextObject) const { return InContextObject; }

	// Called by view model providers when they stop referencing the view model object
	virtual void ShutdownViewModel() {}

	// Creates and initializes a view model instance that is intended to be owned by this view model. Passes along this view model's World Context Object
	// View models must call ShutdownSubViewModels on all SubViewModels in ShutdownViewModel
	UMDViewModelBase* CreateSubViewModel(TSubclassOf<UMDViewModelBase> ViewModelClass, UObject* InContextObject, const FInstancedStruct& ViewModelSettings = FInstancedStruct(), const UObject* WorldContextObject = nullptr) const;
	UMDViewModelBase* CreateSubViewModel(TSubclassOf<UMDViewModelBase> ViewModelClass, const UObject* InContextObject, const FInstancedStruct& ViewModelSettings = FInstancedStruct(), const UObject* WorldContextObject = nullptr) const;
	template<typename T>
	T* CreateSubViewModel(UObject* InContextObject, const FInstancedStruct& ViewModelSettings = FInstancedStruct(), TSubclassOf<UMDViewModelBase> ViewModelClass = T::StaticClass(), const UObject* WorldContextObject = nullptr) const;
	template<typename T>
	T* CreateSubViewModel(const UObject* InContextObject, const FInstancedStruct& ViewModelSettings = FInstancedStruct(), TSubclassOf<UMDViewModelBase> ViewModelClass = T::StaticClass(), const UObject* WorldContextObject = nullptr) const;
	template<typename T, typename U, typename = decltype(TBaseStructure<U>::Get())>
	T* CreateSubViewModel(UObject* InContextObject, const U& ViewModelSettings, TSubclassOf<UMDViewModelBase> ViewModelClass = T::StaticClass(), const UObject* WorldContextObject = nullptr) const;
	template<typename T, typename U, typename = decltype(TBaseStructure<U>::Get())>
	T* CreateSubViewModel(const UObject* InContextObject, const U& ViewModelSettings, TSubclassOf<UMDViewModelBase> ViewModelClass = T::StaticClass(), const UObject* WorldContextObject = nullptr) const;

	// Shuts down a raw pointer to a view model and clears the pointer
	static void ShutdownSubViewModel(UMDViewModelBase*& ViewModel);
	// Shuts down a TObjectPtr to a view model and clears the pointer
	template<typename T>
	static void ShutdownSubViewModel(TObjectPtr<T>& ViewModel);
	// Shuts down a weak object ptr to a view model and resets the ptr
	static void ShutdownSubViewModel(TWeakObjectPtr<UMDViewModelBase>& ViewModelPtr);
	// Iterates an array of View Models, shuts them down and resets the array
	template<typename T>
	static void ShutdownSubViewModels(TArray<T>& ViewModels);
	// Iterates a set of View Models, shuts them down and resets the set
	template<typename T>
	static void ShutdownSubViewModels(TSet<T>& ViewModels);
	// Iterates a map of View Model keys and/or values, shuts them down and resets the map
	template<typename T, typename U>
	static void ShutdownSubViewModels(TMap<T, U>& ViewModels);
	template<typename T, typename U>
	static void ShutdownSubViewModelKeyValues(TMap<T, U>& ViewModels);
	template<typename T, typename U>
	static void ShutdownSubViewModelMapValues(TMap<T, U>& ViewModels);

#if ENGINE_MAJOR_VERSION > 5 || ENGINE_MINOR_VERSION >= 3
	virtual void BroadcastFieldValueChanged(UE::FieldNotification::FFieldId InFieldId) override;
#else
	void BroadcastFieldValueChanged(UE::FieldNotification::FFieldId InFieldId);
#endif

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

	// Specialization for FText since it doesn't implement ==
	bool SetFieldNotifyValue(FText& Value, const FText& NewValue, UE::FieldNotification::FFieldId FieldId);

	// Broadcast that the specified field has changed
	UFUNCTION(BlueprintCallable, Category="FieldNotify", meta = (DisplayName="Broadcast Field Value Changed", ScriptName="BroadcastFieldValueChanged"))
	void K2_BroadcastFieldValueChanged(FFieldNotificationId FieldId);

private:
	// Returns the context object of this view model, casted to the specified type. Will error if the context object is not of the specified type.
	UFUNCTION(BlueprintCallable, Category = "View Model", CustomThunk, meta = (DisplayName = "Get View Model Context Object as Type", DeterminesOutputType = "ContextType", AllowPrivateAccess = "true", ContextType = "/Script/CoreUObject.Object"))
	UObject* GetContextObjectAsType(UPARAM(meta = (AllowAbstract = "true", AllowedClasses = "/Script/CoreUObject.Object,/Script/CoreUObject.Interface")) UClass* ContextType) const;
	DECLARE_FUNCTION(execGetContextObjectAsType);

	const UObject* GetEffectiveWorldContextObject() const;

	UPROPERTY(Transient)
	TWeakObjectPtr<UObject> ContextObject;

	UPROPERTY(Transient)
	TWeakObjectPtr<const UObject> WorldContextObjectPtr;

	UPROPERTY(Transient)
	FInstancedStruct CachedViewModelSettings;

	UPROPERTY(Transient)
	bool bIsInitialized = false;

	UE::FieldNotification::FFieldMulticastDelegate FieldNotifyDelegates;
	TBitArray<> EnabledFieldNotifications;
};

template <typename T, typename U>
FDelegateHandle UMDViewModelBase::AddTypedFieldValueChangedDelegate(UE::FieldNotification::FFieldId InFieldId, TDelegate<void(U)>&& Delegate)
{
	UObject* BoundObject = Delegate.GetUObject();
	auto WrapperDelegate = FFieldValueChangedDelegate::CreateWeakLambda(BoundObject,
		[this, WeakThis = MakeWeakObjectPtr(this), InnerDelegate = MoveTemp(Delegate)](UObject* Object, UE::FieldNotification::FFieldId FieldId)
		{
			if (WeakThis.IsValid())
			{
				T FieldValue;
				if (GetFieldValue(FieldId, FieldValue))
				{
					InnerDelegate.ExecuteIfBound(FieldValue);
				}
			}
		}
	);
	return AddFieldValueChangedDelegate(InFieldId, MoveTemp(WrapperDelegate));
}

template <typename T>
bool UMDViewModelBase::GetFieldValue(UE::FieldNotification::FFieldId FieldId, T& OutValue)
{
	UE::FieldNotification::FFieldVariant Field = FieldId.ToVariant(this);
	if (UFunction* Func = Field.GetFunction())
	{
		checkf(Func->ParmsSize == sizeof(T), TEXT("Param type does not match function return value type"));
		ProcessEvent(Func, &OutValue);

		return true;
	}
	else if (const FProperty* Prop = Field.GetProperty())
	{
		checkf(Prop->GetSize() == sizeof(T), TEXT("Param type does not match field value type"));
		Prop->GetValue_InContainer(this, &OutValue);
		return true;
	}

	return false;
}

template <typename T>
T* UMDViewModelBase::CreateSubViewModel(UObject* InContextObject, const FInstancedStruct& ViewModelSettings, TSubclassOf<UMDViewModelBase> ViewModelClass, const UObject* WorldContextObject) const
{
	static_assert(TIsDerivedFrom<T, UMDViewModelBase>::Value, "ViewModels must derive from UMDViewModelBase");
	return Cast<T>(CreateSubViewModel(ViewModelClass, InContextObject, ViewModelSettings, WorldContextObject));
}

template <typename T>
T* UMDViewModelBase::CreateSubViewModel(const UObject* InContextObject, const FInstancedStruct& ViewModelSettings, TSubclassOf<UMDViewModelBase> ViewModelClass, const UObject* WorldContextObject) const
{
	static_assert(TIsDerivedFrom<T, UMDViewModelBase>::Value, "ViewModels must derive from UMDViewModelBase");
	return Cast<T>(CreateSubViewModel(ViewModelClass, InContextObject, ViewModelSettings, WorldContextObject));
}

template <typename T, typename U, typename>
T* UMDViewModelBase::CreateSubViewModel(UObject* InContextObject, const U& ViewModelSettings, TSubclassOf<UMDViewModelBase> ViewModelClass, const UObject* WorldContextObject) const
{
	static_assert(TIsDerivedFrom<T, UMDViewModelBase>::Value, "ViewModels must derive from UMDViewModelBase");
	return Cast<T>(CreateSubViewModel(ViewModelClass, InContextObject, FInstancedStruct::Make(ViewModelSettings), WorldContextObject));
}

template <typename T, typename U, typename>
T* UMDViewModelBase::CreateSubViewModel(const UObject* InContextObject, const U& ViewModelSettings, TSubclassOf<UMDViewModelBase> ViewModelClass, const UObject* WorldContextObject) const
{
	static_assert(TIsDerivedFrom<T, UMDViewModelBase>::Value, "ViewModels must derive from UMDViewModelBase");
	return Cast<T>(CreateSubViewModel(ViewModelClass, InContextObject, FInstancedStruct::Make(ViewModelSettings), WorldContextObject));
}

template <typename T>
void UMDViewModelBase::ShutdownSubViewModel(TObjectPtr<T>& ViewModel)
{
	if (IsValid(ViewModel))
	{
		ViewModel->ShutdownViewModelFromProvider();
	}

	ViewModel = nullptr;
}

template <typename T>
void UMDViewModelBase::ShutdownSubViewModels(TArray<T>& ViewModels)
{
	for (T& ViewModel : ViewModels)
	{
		ShutdownSubViewModel(ViewModel);
	}

	ViewModels.Reset();
}

template <typename T>
void UMDViewModelBase::ShutdownSubViewModels(TSet<T>& ViewModels)
{
	for (T& ViewModel : ViewModels)
	{
		ShutdownSubViewModel(ViewModel);
	}

	ViewModels.Reset();
}

template <typename T, typename U>
void UMDViewModelBase::ShutdownSubViewModels(TMap<T, U>& ViewModels)
{
	for (TTuple<T, U>& Pair : ViewModels)
	{
		if constexpr (std::is_convertible_v<T, UMDViewModelBase*>|| std::is_convertible_v<T, TWeakObjectPtr<UMDViewModelBase>>)
		{
			ShutdownSubViewModel(Pair.Key);
		}

		if constexpr (std::is_convertible_v<U, UMDViewModelBase*> || std::is_convertible_v<U, TWeakObjectPtr<UMDViewModelBase>>)
		{
			ShutdownSubViewModel(Pair.Value);
		}
	}

	ViewModels.Reset();
}

template <typename T, typename U>
void UMDViewModelBase::ShutdownSubViewModelKeyValues(TMap<T, U>& ViewModels)
{	
	static_assert(std::is_convertible_v<T, UMDViewModelBase*>|| std::is_convertible_v<T, TWeakObjectPtr<UMDViewModelBase>>, "The Map Key type must be a View Model object type");
	
	for (TTuple<T, U>& Pair : ViewModels)
	{
		ShutdownSubViewModel(Pair.Value);
	}

	ViewModels.Reset();
}

template <typename T, typename U>
void UMDViewModelBase::ShutdownSubViewModelMapValues(TMap<T, U>& ViewModels)
{
	static_assert(std::is_convertible_v<U, UMDViewModelBase*> || std::is_convertible_v<U, TWeakObjectPtr<UMDViewModelBase>>, "The Map Value type must be a View Model object type");
	
	for (TTuple<T, U>& Pair : ViewModels)
	{
		ShutdownSubViewModel(Pair.Value);
	}

	ViewModels.Reset();
}
