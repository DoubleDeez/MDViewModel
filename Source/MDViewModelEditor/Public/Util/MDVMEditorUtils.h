#pragma once

#include "UObject/NameTypes.h"

class UMDViewModelBase;

namespace MDVMEditorUtils
{
	// Meta tag used to hide classes/properties/etc from custom MDViewModel editor UI
	MDVIEWMODELEDITOR_API extern const FName VMHiddenMeta;

	MDVIEWMODELEDITOR_API void SaveViewModelConfig(UMDViewModelBase* ViewModelCDO);
};
