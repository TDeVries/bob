#include "ScaleExplorer.h"
#include "ipSWPruner.h"
#include "ipSWEvaluator.h"
#include "Image.h"

namespace Torch {

/////////////////////////////////////////////////////////////////////////
// Constructor

ScaleExplorer::ScaleExplorer()
	:	m_sw_size()
{
}

/////////////////////////////////////////////////////////////////////////
// Destructor

ScaleExplorer::~ScaleExplorer()
{
}

/////////////////////////////////////////////////////////////////////////
// Initialize the scanning process (scanning sub-window size, ROI)

bool ScaleExplorer::init(int sw_w, int sw_h, const sRect2D& roi)
{
	// Check parameters
	if (	sw_w < 1 || sw_h < 1 || sw_w > roi.w || sw_h > roi.h ||
		roi.x < 0 || roi.y < 0)
	{
		Torch::message("ScaleExplorer::init - invalid parameters!\n");
		return false;
	}

	// OK
	m_sw_size.w = sw_w;
	m_sw_size.h = sw_h;
	m_roi = roi;
	return true;
}

/////////////////////////////////////////////////////////////////////////
// Process some sub-window

bool ScaleExplorer::processSW(int sw_x, int sw_y, int sw_w, int sw_h, ExplorerData& explorerData)
{
        // Check if the subwindow should be prunned ...
	for (int i = 0; i < explorerData.m_nSWPruners; i ++)
	{
	        ipSWPruner* swPruner = explorerData.m_swPruners[i];

                if (swPruner->setSubWindow(sw_x, sw_y, sw_w, sw_h) == false)
		{
			Torch::message("ScaleExplore::processSW - error calling some pruner!\n");
			return false;
		}

		// If rejected, then there is no point in running the pattern model!
		if (swPruner->isRejected() == true)
		{
			explorerData.m_stat_prunned ++;	// Update statistics
			return true;
		}
	}

	// Not rejected, so run the pattern model (evaluator) on this sub-window
	ipSWEvaluator* swEvaluator = explorerData.m_swEvaluator;
	if (swEvaluator->setSubWindow(sw_x, sw_y, sw_w, sw_h) == false)
	{
		Torch::message("ScaleExplorer::processSW - error calling the evaluator!\n");
		return false;
	}

	// OK, update statistics and keep the sub-window if accepted
	explorerData.m_stat_scanned ++;
	if (swEvaluator->isPattern() == true)
	{
		explorerData.m_stat_accepted ++;

		// Add the sub-window to the pattern space
		// NB: call the <storePattern> from Explorer::Data and not add it directly
		//	(for pyramid scanning approach, these sub-windows may need rescalling first,
		//	and <storePattern> will take care of this)
		explorerData.storePattern(
				swEvaluator->getSubWindowX(),
				swEvaluator->getSubWindowY(),
				swEvaluator->getSubWindowW(),
				swEvaluator->getSubWindowH(),
				swEvaluator->getConfidence());
	}

	return true;
}

/////////////////////////////////////////////////////////////////////////

}
