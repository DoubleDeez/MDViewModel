#pragma once

#include "Styling/CoreStyle.h"
#include "Framework/Commands/Commands.h"

class MDVIEWMODELEDITOR_API FMDViewModelEditorCommands : public TCommands<FMDViewModelEditorCommands>
{
public:
	FMDViewModelEditorCommands()
		: TCommands<FMDViewModelEditorCommands>(TEXT("MDViewModelEditorCommands"), NSLOCTEXT("MDViewModelEditorCommands", "ViewModelEditorCommands", "View Model Commands"), NAME_None, FCoreStyle::Get().GetStyleSetName())
	{}

	virtual void RegisterCommands() override;

	TSharedPtr<FUICommandInfo> Edit;
};
