#pragma once

#include "IPropertyTypeCustomization.h"
#include "Input/Reply.h"
#include "SGraphPin.h"
#include "Util/MDViewModelAssignment.h"

class SWidget;
struct FMDViewModelAssignmentReference;

/**
 * Customizes FMDViewModelAssignmentReference properties to display a selector for the viewmodel
 */
class FMDViewModelAssignmentReferenceCustomization : public IPropertyTypeCustomization
{
public:
	static TSharedRef<IPropertyTypeCustomization> MakeInstance();

	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils) override;
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils) override;

protected:
	FMDViewModelAssignmentReference* GetAssignmentReference() const;
	UClass* GetWidgetOwnerClass() const;

	TSharedRef<SWidget> MakeAssignmentMenu();
	void SetSelectedAssignment(FMDViewModelAssignment Assignment) const;

	FText GetSelectedAssignmentText() const;

	TSharedPtr<IPropertyHandle> StructHandle;
	TSharedPtr<IPropertyHandle> ClassHandle;
};

/**
 * Customizes FMDViewModelAssignmentReference pins to display a selector for the viewmodel
 */
class SMDViewModelAssignmentReferenceGraphPin : public SGraphPin
{
public:
	using SGraphPin::Construct;

	virtual TSharedRef<SWidget>	GetDefaultValueWidget() override;
	virtual void Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime ) override;

private:
	void GetWidgetViewModelAssignments(TMap<FMDViewModelAssignment, FMDViewModelAssignmentData>& OutViewModelAssignments) const;
	const UEdGraphPin* GetWidgetPin() const;
	const UEdGraphPin* GetViewModelPin() const;

	TSubclassOf<UUserWidget> GetConnectedWidgetClass() const;
	TSubclassOf<UMDViewModelBase> GetConnectedViewModelClass() const;

	TSharedRef<SWidget> MakeAssignmentMenu();
	void SetSelectedAssignment(FMDViewModelAssignment Assignment) const;

	FMDViewModelAssignmentReference GetSelectedAssignment() const;
	FText GetSelectedAssignmentText() const;

	void ValidateDefaultValue() const;
};
