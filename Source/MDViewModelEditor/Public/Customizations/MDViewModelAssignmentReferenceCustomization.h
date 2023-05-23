#pragma once

#include "CoreMinimal.h"
#include "IPropertyTypeCustomization.h"
#include "Input/Reply.h"
#include "Util/MDViewModelAssignment.h"

class SWidget;
struct FMDViewModelAssignmentReference;

/**
 * Customizes FMDViewModelAssignmentReference to display a selector for the viewmodel
 */
class MDVIEWMODELEDITOR_API FMDViewModelAssignmentReferenceCustomization : public IPropertyTypeCustomization
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
