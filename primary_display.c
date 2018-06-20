void DSI_set_cmdq_V2_Wrapper_DSI0(unsigned cmd, unsigned char count, unsigned char *para_list, unsigned char force_update);
#define read_reg_v2(cmd, buffer, buffer_size) DSI_dcs_read_lcm_reg_v2(DISP_MODULE_DSI0, NULL, cmd, buffer, buffer_size)
#define dsi_set_cmdq_V2(cmd, size, data) DSI_set_cmdq_V2_Wrapper_DSI0(cmd, size, data, 1)

/* 
 * mtk_dsi_write_regs: mipi write registers
 * @cmd: operate command
 * @data: data buffer to write, can be null
 * @size: data buffer size, can be 0
 * 
 * Return: 0 success, otherwise fail
 */
int mtk_dsi_write_regs(char cmd, char *data, int size)
{
	dsi_set_cmdq_V2(cmd, size, data);
	return 0;
}

/* 
 * mtk_dsi_read_regs: mipi read registers
 * @cmd: operate command
 * @buffer: buffer to save return values, cannot be null
 * @size: buffer size, cannot be 0
 * 
 * Return: 0 success, otherwise fail
 */
int mtk_dsi_read_regs(char cmd, char *buffer, int size)
{
	int ret = 0;

	_primary_path_lock(__func__);
	if (pgc->state == DISP_SLEPT) {
		pr_err("[mtk]primary display path is slept?? -- skip esd check\n");
		_primary_path_unlock(__func__);
		goto done;
	}
	/* 1: stop path */
	_cmdq_stop_trigger_loop();
	if (dpmgr_path_is_busy(pgc->dpmgr_handle))
		pr_debug("[mtk]wait frame done ret:%d\n", ret);

	dpmgr_path_stop(pgc->dpmgr_handle, CMDQ_DISABLE);
	if (dpmgr_path_is_busy(pgc->dpmgr_handle))
		DISPCHECK("[ESD]wait frame done ret:%d\n", ret);

	dpmgr_path_reset(pgc->dpmgr_handle, CMDQ_DISABLE);

	ret = read_reg_v2(cmd, buffer, size);
	if (ret != size) {
		ret = -EIO;
	} else {
		ret = 0;
	}

	dpmgr_path_start(pgc->dpmgr_handle, CMDQ_DISABLE);
	if (primary_display_is_video_mode()) {
		/* for video mode, we need to force trigger here */
		/* for cmd mode, just set DPREC_EVENT_CMDQ_SET_EVENT_ALLOW when trigger loop start */
		dpmgr_path_trigger(pgc->dpmgr_handle, NULL, CMDQ_DISABLE);
	}
	_cmdq_start_trigger_loop();
	/* when we stop trigger loop
	 * if no other thread is running, cmdq may disable its clock
	 * all cmdq event will be cleared after suspend */
	cmdqCoreSetEvent(CMDQ_EVENT_DISP_WDMA0_EOF);
	
	_primary_path_unlock(__func__);

done:
	return ret;
}

