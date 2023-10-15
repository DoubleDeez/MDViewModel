#pragma once

#include "MDViewModelBase.h"
#include "MDViewModelBlueprintBase.generated.h"

/**
 * Base class for creating view model blueprints.
 */
UCLASS(Abstract, Blueprintable)
class MDVIEWMODEL_API UMDViewModelBlueprintBase : public UMDViewModelBase
{
	GENERATED_BODY()

public:
	virtual UWorld* GetWorld() const override;

	virtual void PostInitProperties() override;

	virtual const UE::FieldNotification::IClassDescriptor& GetFieldNotificationDescriptor() const override;

	struct MDVIEWMODEL_API FFieldNotificationClassDescriptor : public UMDViewModelBase::FFieldNotificationClassDescriptor
	{
		virtual void ForEachField(const UClass* Class, TFunctionRef<bool(UE::FieldNotification::FFieldId FieldId)> Callback) const override;
	};

#if WITH_EDITOR && WITH_EDITORONLY_DATA
	virtual UScriptStruct* GetViewModelSettingsStruct() const override { return ViewModelSettingsType; }
	virtual void GetSupportedContextObjectTypes(const FInstancedStruct& ViewModelSettings, UBlueprint* Blueprint, TArray<TSubclassOf<UObject>>& OutClasses) const override { OutClasses = SupportedContextObjectTypes; }
	virtual void GetStoredContextObjectTypes(const FInstancedStruct& ViewModelSettings, UBlueprint* Blueprint, TArray<TSubclassOf<UObject>>& OutClasses) const override;

#if ENGINE_MAJOR_VERSION > 5 || ENGINE_MINOR_VERSION >= 3
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#else
	virtual EDataValidationResult IsDataValid(TArray<FText>& ValidationErrors) override;
#endif
#endif

protected:
	// Redirect from the provided Context Object. The result will be the result of GetContextObject for this view model's life time.
	UFUNCTION(BlueprintImplementableEvent, Category = "View Model", DisplayName = "Redirect Context Object", meta = (MDVMHidden))
	UObject* BP_RedirectContextObject(UObject* ProvidedContextObject) const;
	virtual UObject* RedirectContextObject(UObject* ProvidedContextObject) const override;

	// Redirect from the configured Context Object lifetime. Instances of this view model will be cached on the result.
	UFUNCTION(BlueprintImplementableEvent, Category = "View Model", DisplayName = "Redirect Cached Context Object (CDO Only)", meta = (MDVMHidden))
	UObject* BP_CDORedirectCachedContextObject(UObject* ProvidedCacheContextObject, const FInstancedStruct& ViewModelSettings) const;
	virtual UObject* CDORedirectCachedContextObject(const UObject* WorldContextObject, UObject* ProvidedCacheContextObject, const FInstancedStruct& ViewModelSettings) const override;

	// Bind to the context object, set initial property values and perform any other necessary initialization
	UFUNCTION(BlueprintImplementableEvent, Category = "View Model", DisplayName = "Initialize View Model", meta = (MDVMHidden))
	void BP_InitializeViewModel();
	virtual void InitializeViewModel() override;

	// Unbind any delegates and perform any other necessary uninitialization
	UFUNCTION(BlueprintImplementableEvent, Category = "View Model", DisplayName = "Shutdown View Model", meta = (MDVMHidden))
	void BP_ShutdownViewModel();
	virtual void ShutdownViewModel() override;

	// Creates and initializes a view model instance that is intended to be owned by this view model. Passes along this view model's World Context Object
	UFUNCTION(BlueprintCallable, Category = "View Model", meta = (AutoCreateRefTerm = "ViewModelSettings", DeterminesOutputType = "ViewModelClass"))
	UMDViewModelBase* CreateSubViewModel(TSubclassOf<UMDViewModelBase> ViewModelClass, UObject* InContextObject, const FInstancedStruct& ViewModelSettings) const;

#if WITH_EDITORONLY_DATA
	// Specify the struct type to expose in the view model assignment editor
	UPROPERTY(EditDefaultsOnly, Category = "View Model")
	TObjectPtr<UScriptStruct> ViewModelSettingsType;

	// Specify which types of context objects this view model can be initialized from.
	// Used for displaying validation messages in the view model editor.
	// Add a 'None' entry to indicate that a Context Object is not required.
	// Add 'Object' to indicate that any type of valid Context Object is supported.
	UPROPERTY(EditDefaultsOnly, Category = "View Model", meta = (AllowAbstract, AllowedClasses = "/Script/CoreUObject.Object,/Script/CoreUObject.Interface"))
	TArray<TSubclassOf<UObject>> SupportedContextObjectTypes;

	// If overriding "Redirect Context Object" and/or "Redirect Cached Context Object", specify the context object type that will be returned from either functions.
	UPROPERTY(EditDefaultsOnly, Category = "View Model", meta = (AllowAbstract, AllowedClasses = "/Script/CoreUObject.Object,/Script/CoreUObject.Interface"))
	TArray<TSubclassOf<UObject>> StoredContextObjectTypes;
#endif

private:
	UPROPERTY(Transient)
	bool bImplements_RedirectContextObject;
	UPROPERTY(Transient)
	bool bImplements_CDORedirectCachedContextObject;
	UPROPERTY(Transient)
	bool bImplements_InitializeViewModel;
	UPROPERTY(Transient)
	bool bImplements_ShutdownViewModel;

	UPROPERTY(Transient)
	mutable TArray<TWeakObjectPtr<UMDViewModelBase>> SubViewModels;

	UPROPERTY(Transient)
	mutable TWeakObjectPtr<const UObject> CDOWorldContextObjectPtr;
};
