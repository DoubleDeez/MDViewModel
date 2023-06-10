#pragma once

#include "Components/WidgetSwitcher.h"
#include "UObject/Object.h"
#include "MDViewModelSwitcher.generated.h"

class UMDViewModelBase;
/**
 * Widget that will switch to the first child that accepts the provided view model
 * Using this widget requires knowledge of which types of viewmodels will be passed to it to be used effectively.
 */
UCLASS()
class MDVIEWMODEL_API UMDViewModelSwitcher : public UWidgetSwitcher
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "View Model")
	void SetViewModel(UMDViewModelBase* ViewModel);

#if WITH_EDITOR
	virtual const FText GetPaletteCategory() override;
	virtual void ValidateCompiledDefaults(IWidgetCompilerLog& CompileLog) const override;
#endif

protected:
	virtual UClass* GetSlotClass() const override;
};
