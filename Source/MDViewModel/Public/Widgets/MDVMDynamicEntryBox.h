#pragma once

#include "Components/DynamicEntryBox.h"
#include "Util/MDViewModelAssignmentReference.h"
#include "MDVMDynamicEntryBox.generated.h"

class UMDViewModelBase;

/**
 * A dynamic entry box that can be populated by a list of view models
 */
UCLASS(meta = (DisplayName = "Dynamic Entry Box (View Model)"))
class MDVIEWMODEL_API UMDVMDynamicEntryBox : public UDynamicEntryBox
{
	GENERATED_BODY()

public:
	virtual void PostInitProperties() override;

#if WITH_EDITOR
	virtual const FText GetPaletteCategory() override;
	virtual void ValidateCompiledDefaults(class IWidgetCompilerLog& CompileLog) const override;
#endif

	UFUNCTION(BlueprintCallable, Category = "DynamicEntryBox")
	void PopulateItems(const TArray<UMDViewModelBase*>& ViewModels);

protected:
	UPROPERTY(EditAnywhere, Category = "View Model")
	FMDViewModelAssignmentReference ViewModelAssignment;

private:
#if WITH_EDITOR
	UClass* GetEditorTimeEntryWidgetClass() const;
#endif

};
