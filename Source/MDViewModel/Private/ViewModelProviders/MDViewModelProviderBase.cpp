#include "ViewModelProviders/MDViewModelProviderBase.h"


void FMDViewModelProviderBase::BroadcastViewModelUpdated(TSubclassOf<UMDViewModelBase> ViewModelClass) const
{
	OnViewModelUpdated.Broadcast(ViewModelClass);
}
