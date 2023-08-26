#include "MDViewModelEditorConfig.h"

#include "ClassViewerModule.h"

UMDViewModelEditorConfig::UMDViewModelEditorConfig()
{
	CategoryName = TEXT("Plugins");
	SectionName = TEXT("View Model Editor");
}

EClassViewerNameTypeToDisplay UMDViewModelEditorConfig::GetNameTypeToDisplay() const
{
	switch (NameTypeToDisplay)
	{
	case EMDVMClassViewerNameTypeToDisplay::Dynamic:
		return EClassViewerNameTypeToDisplay::Dynamic;
	case EMDVMClassViewerNameTypeToDisplay::ClassName:
		return EClassViewerNameTypeToDisplay::ClassName;
	case EMDVMClassViewerNameTypeToDisplay::DisplayName:
		return EClassViewerNameTypeToDisplay::DisplayName;
	default:
		checkf(false, TEXT("Unhandled EMDVMClassViewerNameTypeToDisplay type"));
	}

	return EClassViewerNameTypeToDisplay::DisplayName;
}
