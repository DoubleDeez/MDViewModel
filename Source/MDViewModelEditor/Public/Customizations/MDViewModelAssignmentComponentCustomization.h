#pragma once

#include "IDetailCustomization.h"
#include "Input/Reply.h"

struct FMDViewModelAssignment;
struct FMDViewModelAssignmentData;
class UMDViewModelAssignmentComponent;

/**
 * Customization for UMDViewModelAssignmentComponent
 */
class FMDViewModelAssignmentComponentCustomization : public IDetailCustomization
{
public:
	static TSharedRef<IDetailCustomization> MakeInstance()
	{
		return MakeShared<FMDViewModelAssignmentComponentCustomization>();
	}

	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

	virtual void CustomizeDetails(const TSharedPtr<IDetailLayoutBuilder>& DetailBuilder) override
	{
		CachedDetailBuilder = DetailBuilder;
		CustomizeDetails(*DetailBuilder);
	}

private:
	void HideDefaultCategories();
	void TryAddViewModelEditorButton();
	void AddViewModelDetails();

	FReply OpenViewModelEditorTab() const;

	UMDViewModelAssignmentComponent* GetComponent() const;
	
	void RequestRefresh();
	
	TWeakPtr<IDetailLayoutBuilder> CachedDetailBuilder;
};
