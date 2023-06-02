hd_audio_capture_only:
	[data flow] audiocap -> (user)
	[usage]
	(1) require input files: (none).
	(2) require microphone.
	(3) execute hd_audio_capture_only, it will start with above data flow.
	(4) press 'q' to quit this app.
	(5) enter "sync" to ensure all files are saved.
	(6) generate output files: check them on /mnt/sd/audio_bs_16_2_48000_pcm.dat and /mnt/sd/audio_bs_16_2_48000_pcm.len.

hd_audio_capture_only_with_2cap:
        [data flow] audiocap -> (user)
        [usage]
        (1) require input files: (none).
        (2) require microphone.
        (3) execute hd_audio_capture_only_with_2cap, it will start with above data flow.
        (4) press 'q' to quit this app.
        (5) enter "sync" to ensure all files are saved.
        (6) generate output files: check them on /mnt/sd/audio_bs_16_1_48000_pcm_p0.dat, /mnt/sd/audio_bs_16_1_48000_pcm_p0.len, /mnt/sd/audio_bs_16_1_48000_pcm_p1.dat and /mnt/sd/audio_bs_16_1_48000_pcm_p1.len.

hd_audio_decode_only:
	[data flow] (user) -> audiodec -> (user)
	[usage]
	(1) require input files: /pattern/audio_bs_16_2_48000_aac.dat and /pattern/audio_bs_16_2_48000_aac.len,
		copy them to /mnt/sd/audio_bs_16_2_48000_aac.dat and /mnt/sd/audio_bs_16_2_48000_aac.len.
	(2) execute hd_audio_decode_only [dec_type], it will start with above data flow.
	(3) press 'q' to quit this app.
	(4) enter "sync" to ensure all files are saved.
	(5) generate output files: check them on /mnt/sd/dump_frm_16_2_48000_pcm.dat and /mnt/sd/dump_frm_16_2_48000_pcm.len.
	NOTE: [dec_type] 0:AAC 1:ULAW 2:ALAW 3:PCM.

hd_audio_encode_only:
	[data flow] (user) -> audioenc -> (user)
	[usage]
	(1) require input files: /pattern/audio_frm_16_2_48000_pcm.dat and /pattern/audio_frm_16_2_48000_pcm.len,
		copy them to /mnt/sd/audio_frm_16_2_48000_aac.dat and /mnt/sd/audio_frm_16_2_48000_aac.len.
	(2) execute hd_audio_encode_only [enc_type], it will start with above data flow.
	(3) press 'q' to quit this app.
	(4) enter "sync" to ensure all files are saved.
	(5) generate output files: check them on /mnt/sd/dump_bs_16_2_48000_aac.dat and /mnt/sd/dump_bs_16_2_48000_aac.len.
	NOTE: [enc_type] 0:AAC 1:ULAW 2:ALAW.

hd_audio_livesound:
	[data flow] audiocap -> audioout
	[usage]
	(1) require input files: (none).
	(2) require microphone and speaker.
	(3) execute hd_audio_livesound, it will start with above data flow.
	(4) check the sound played by the speaker.
	(5) press 'q' to quit this app.

hd_audio_output_only:
	[data flow] (user) -> audioout
	[usage]
	(1) require input files: /pattern/audio_frm_16_2_48000_pcm.dat and /pattern/audio_frm_16_2_48000_pcm.len,
		copy them to /mnt/sd/audio_frm_16_2_48000_pcm.dat and /mnt/sd/audio_frm_16_2_48000_pcm.len.
	(2) require speaker.
	(3) execute hd_audio_output_only, it will start with above data flow.
	(4) check the sound played by the speaker.
	(5) press 'q' to quit this app.

hd_audio_output_only_with_2out:
	[data flow] (user) -> audioout
	[usage]
	(1) require input files: /pattern/audio_frm_16_1_48000_pcm.dat and /pattern/audio_frm_16_1_48000_pcm.len,
		copy them to /mnt/sd/audio_frm_16_1_48000_pcm.dat and /mnt/sd/audio_frm_16_1_48000_pcm.len.
	(2) require speaker.
	(3) execute hd_audio_output_only_with_2out, it will start with above data flow.
	(4) check the sound played by the speaker.
	(5) press 'q' to quit this app.

hd_audio_playback:
	[data flow] (user) -> audiodec -> audioout
	[usage]
	(1) require input files: /pattern/audio_bs_16_2_48000_aac.dat and /pattern/audio_bs_16_2_48000_aac.len,
		copy them to /mnt/sd/audio_bs_16_2_48000_aac.dat and /mnt/sd/audio_bs_16_2_48000_aac.len.
	(2) require speaker.
	(3) execute hd_audio_playback [dec_type], it will start with above data flow.
	(4) check the sound played by the speaker.
	(5) press 'q' to quit this app.
	NOTE: [dec_type] 0:AAC 1:ULAW 2:ALAW 3:PCM.

hd_audio_record:
	[data flow] audiocap -> audioenc -> (user)
	[usage]
	(1) require input files: (none).
	(2) require microphone.
	(3) execute hd_audio_record [enc_type], it will start with above data flow.
	(4) press 'q' to quit this app.
	(5) enter "sync" to ensure all files are saved.
	(6) generate output files: check them on /mnt/sd/dump_bs_16_2_48000_aac.dat and /mnt/sd/dump_bs_16_2_48000_aac.len.
	NOTE: [enc_type] 0:AAC 1:ULAW 2:ALAW.

hd_audio_bidirect_with_aec:
	[data flow] audiocap -> (user), (user) -> audioout
	[usage]
	(1) require input files: audio_bs_16_1_16000_pcm_out.dat and audio_bs_16_1_16000_pcm_out.len,
		copy them to /mnt/sd/audio_bs_16_1_16000_pcm_out.dat and /mnt/sd/audio_bs_16_1_16000_pcm_out.len.
	(2) require microphone and speaker.
	(3) execute hd_audio_bidirect_with_aec, it will start with above data flow.
	(4) check the sound played by the speaker.
	(5) press 'q' to quit this app.
	(6) enter "sync" to ensure all files are saved.
	(7) generate output files: check them on /mnt/sd/audio_bs_16_1_16000_pcm_cap.dat and /mnt/sd/audio_bs_16_1_16000_pcm_cap.len.

hd_audio_2bidirect_with_aec:
	[data flow] audiocap -> (user), (user) -> audioout
	[usage]
	(1) require input files: audio_bs_16_1_16000_pcm_out_p0.dat, audio_bs_16_1_16000_pcm_out_p0.len,
		audio_bs_16_1_16000_pcm_out_p1.dat andaudio_bs_16_1_16000_pcm_out_p1.len.
		copy them to /mnt/sd/audio_bs_16_1_16000_pcm_out_p0.dat and /mnt/sd/audio_bs_16_1_16000_pcm_out_p0.len,
		/mnt/sd/audio_bs_16_1_16000_pcm_out_p1.dat and /mnt/sd/audio_bs_16_1_16000_pcm_out_p1.len
	(2) require microphone and speaker.
	(3) execute hd_audio_2bidirect_with_aec, it will start with above data flow.
	(4) check the sound played by the speaker.
	(5) press 'q' to quit this app.
	(6) enter "sync" to ensure all files are saved.
	(7) generate output files: check them on /mnt/sd/audio_bs_16_1_16000_pcm_cap_p0.dat and /mnt/sd/audio_bs_16_1_16000_pcm_cap_p0.len,
		/mnt/sd/audio_bs_16_1_16000_pcm_cap_p1.dat and /mnt/sd/audio_bs_16_1_16000_pcm_cap_p1.len.

hd_common:
	[data flow] none.
	[usage]
	(1) require input files: (none).
	(2) Auto test memory APIs and check if pass, if any item fail will break the test program and print error.
	(3) The auto test items are below:
		[1] test get block from common pool
	  	[2] test get block from osg pool
	  	[3] test alloc/free continuous memory
	  	[4] test translate user space va to pa
	  	[5] test if user space va to pa still work at the case "multiple va mmap to the same physical"
	(4) press 'q' to quit this app.

hd_debug_test:

hd_gfx_only:
	[data flow] (user) -> gfx -> (user)
	[usage]
	(1) require input files: /pattern/video_frm_1000_200_1_argb4444.dat, video_frm_1920_1080_1_yuv420.dat video_frm_1920_1080_1_yuv4203P.dat.
		copy it to /mnt/sd/video_frm_1920_1080_1_yuv420.dat, video_frm_1920_1080_1_yuv420.dat, video_frm_1920_1080_1_yuv4203P.dat.
	(2) execute hd_gfx_only.
	(3) press 'c' to test copy
		press 's' to test scale
		press 'o' to test rotation
		press 't' to test color transform
		press 'l' to test draw line
		press 'r' to test draw rectangle
		press 'm' to test dma memory copy
		press 'a' to test arithmetic
		press 'j' to test job/list
		press 'p' to transfrom yuv420 3p to 2p
		press 'd' to scale up yuv420 3p
    	(4) copy test generates /mnt/sd/hd_gfx_copy_1920_1080_1_yuv420.dat : a 1000x200 novatek logo is printed on 1920x1080 image
		scale test generates /mnt/sd/hd_gfx_scale_3840_2160_1_yuv420.dat : a 1920x1080 image is scaled up to 3840x2160
		rotation test generates /mnt/sd/hd_gfx_rotate_1920_1080_1_yuv420.dat : the input image is rotated by 180 degree
		color transform test generates /mnt/sd/hd_gfx_color_transform_1000_200_1_yuv420.dat : the input argb4444 image is tranformed to yuv420
		draw line test generates /mnt/sd/hd_gfx_draw_line_1920_1080_1_yuv420.dat : a green line is painted on the image
		draw rectangle test generates /mnt/sd/hd_gfx_draw_rect_1920_1080_1_yuv420.dat : a green rectangle is painted on the image
		dma memory copy test prints "dma manages to copy 1048576 bytes" on success
		arithmetic test prints "minus operation of two 16bits arrays ok" on success
		job/list test generates /mnt/sd/hd_gfx_job_list_1920_1080_1_yuv420.dat : a frame and 4 rectangles are painted on the image
		transfrom yuv420 3p to 2p test generates /mnt/sd/hd_gfx_3p_to_2p_1920_1080_1_yuv420.dat : the input yuv420 3p image is tranformed to yuv420 2p
		scale yuv420 3p test generates /mnt/sd/hd_gfx_scale_420_3p_480_270_1_yuv4203p.dat : a 1920x1080 yuv420 3p image is scaled down to 480x270
	(5) enter "sync" to ensure all output files are fully saved.

hd_video_capture_only:
	[data flow] videocap -> (user)
	[usage]
	(1) execute hd_video_capture_only, it will start with above data flow.
	(2) press 's' to save a raw file to /mnt/sd/cap?.raw.
	(3) press 'q' to quit this app.
	(4) enter "sync" to ensure all files are saved.

hd_video_decode_only:
	[data flow] (user) -> videodec-> (user)
	[usage]
	(1) require input files: /pattern/video_bs_640_480_h265.dat, copy it to /mnt/sd/video_bs_640_480_h265.dat.
	(2) execute hd_video_decode_only [dec_type], it will start with above data flow.
	(3) press 'q' to quit this app.
	(4) enter "sync" to ensure all files are saved.
	(5) generate output files: check it on /mnt/sd/dump_frm_640_480_yuv.dat.
	NOTE: [dec_type] 0:H265 1:H264 2:JPEG

hd_video_encode_only:
	[data flow] (user) -> videoenc-> (user)
	[usage]
	(1) require input files: /pattern/video_frm_352_288_30_yuv420.dat, copy it to /mnt/sd/video_frm_352_288_30_yuv420.dat.
	(2) execute hd_video_encode_only, it will start with above data flow.
	(3) press 'q' to quit this app.
	(4) enter "sync" to ensure all files are saved.
	(5) generate output files: check it on /mnt/sd/dump_bs_main.dat.

hd_video_liveview:
	[data flow] videocap -> videoproc -> videoout
	[usage]
	(1) require input files: (none).
	(2) require sensor board imx291 and display
	(3) execute hd_video_liveview, it will start with above data flow.
	(4) check the video result on display.
	(5) press 'q' to quit this app.


hd_video_liveview_with_dir:
	[data flow] videocap -> videoproc -> videoout
	[usage]
	(1) require input files: (none).
	(2) require sensor board imx291 and display
	(3) execute hd_video_liveview_with_dir, it will start with above data flow.
	(4) press 'q' to quit this app.
		press 'w or x' to dynamic change videocap output mirror/flip (only support CCIR sensor)
		press 'e or c' to dynamic change videoproc input mirror/flip
		press 't or b' to dynamic change videoout input mirror/flip
	(5) check the video result on display.

hd_video_liveview_with_vcap_2dev:
	[data flow] videocap -> videoproc -> videoout
	[usage]
	(1) require dual sensor board imx290 and LCD.
	(2) disable "modprobe nvt_sen_imx290 sen_cfg_path=/mnt/app/sensor/sen_imx290.cfg" and enable "modprobe nvt_sen_imx290 sen_cfg_path=/mnt/app/sensor/sen_imx290_dual.cfg" in the script file, S10_SysInit2.
	(3) execute hd_video_liveview_with_vcap_2dev [out_type], it will start with above data flow.
	NOTE: [out_type] 0:cap0 ->vout0 1:cap1->vout0 2:cap0+cap1 ->vout0(pip) 3:cap0+cap1 ->vout0(side by side)
hd_video_liveview_with_frc:
	[data flow] videocap -> videoproc -> videoout
	[usage]
	(1) require input files: (none).
	(2) require sensor board imx291 and display
	(3) execute hd_video_liveview_with_frc, it will start with above data flow.
	(4) press 'q' to quit this app.
		press 'e or c' to dynamic change videoproc input frame rate
		press 'r or v' to dynamic change videoproc output frame rate
	(5) check the video result on display.

hd_video_liveview_with_pat:
	[data flow] videocap -> videoproc -> videoout
	[usage]
	(1) require input files: (none).
	(2) execute hd_video_liveview_with_pat [pat_mode] [pat_value]
	(3) check the video result on LCD.
	(4) press 'q' to quit this app .
	NOTE: [pat_mode] [pat_value] referring to HD_VIDEOCAP_SEN_PATGEN_SEL.

hd_video_liveview_with_osg:
	[data flow] videocap -> videoproc -> videoout
	[usage]
	(1) require input files: /pattern/video_frm_1000_200_1_argb4444.dat, copy it to /mnt/sd/video_frm_1000_200_1_argb4444.dat.
	(2) require sensor board imx291 and display.
	(3) execute hd_video_liveview_with_osg [out_type], it will start with above data flow.
	(4) LCD should display streaming with a novatek logo and a red rectangle.
	(5) press 'q' to quit this app.
	NOTE: [out_type] 0:NTSC 1:EVB lcd (Disp_IF8B_LCD1_PW35P00_HX8238D) 2:HDMI.
	NOTE: execute cat /proc/hdmitx/help would list support hdmi id, when hdmi is attathed


hd_video_liveview_with_wdr_defog:
	[data flow] videocap -> videoproc -> videoout
	[usage]
	(1) require input files: (none).
	(2) require sensor board imx291 and display
	(3) execute hd_video_liveview_with_wdr_defog, it will start WDR and DEFOG effect with above data flow.
	(4) check the video result on display.
	(5) press 'q' to quit this app.

hd_video_output_only:
	[data flow] (user) -> videoout
	[usage]
	(1) require input files: /pattern/video_frm_320_240_3_yuv422.dat, copy it to /mnt/sd/video_frm_320_240_3_yuv422.dat.
	(2) require display
	(3) execute hd_video_output_only [out_type] [hdmi_id], it will start with above data flow.
	(4) check the video result on display.
	(5) press 'q' to quit this app.
	NOTE: [out_type] 0:NTSC 1:EVB lcd (Disp_IF8B_LCD1_PW35P00_HX8238D) 2:HDMI.
	NOTE: execute cat /proc/hdmitx/help would list support hdmi id, when hdmi is attathed

hd_video_playback:
	[data flow] (user) -> videodec -> videoproc -> videoout
	[usage]
	(1) require input files: /pattern/video_bs_640_480_h265.dat and /pattern/video_bs_640_480_h265.len,
		copy them to /mnt/sd/video_bs_640_480_h265.dat and /pattern/video_bs_640_480_h265.len.
	(2) require display
	(3) execute hd_video_playback [dec_type], it will start with above data flow.
	(4) check the video result on display.
	(4) press 'q' to quit this app.
	NOTE: [dec_type] 0:H265 1:H264 2:JPEG

hd_video_process_only:
	[data flow] (user) -> videoproc -> (user)
	[usage] TBD.
	(1) require input files: /pattern/video_frm_352_288_30_yuv420.dat, copy it to /mnt/sd/video_frm_352_288_30_yuv420.dat.
	(2) execute hd_video_process_only, it will start with above data flow.
	(3) press 's' to save a raw file to /mnt/sd/dump_frm_yuv420_xxx.dat.
	(4) press 'q' to quit this app.
	(5) enter "sync" to ensure all files are saved.
	

hd_video_record:
	[data flow] videocap -> videoproc -> videoenc -> (user)
	[usage]
	(1) require input files: (none).
	(2) require sensor board imx291.
	(3) execute hd_video_record [enc_type], it will start with above data flow.
	(4) press 'q' to quit this app.
	(5) enter "sync" to ensure all files are saved.
	(6) generate output files: check it on /mnt/sd/dump_bs_main.dat.
	NOTE: [enc_type] 0:H265 1:H264 2:JPEG

hd_video_record_with_2proc:
	[data flow] videocap -> videoproc -> videoproc -> videoenc -> (user)
	[usage]
	(1) require input files: (none).
	(2) require sensor board imx291.
	(3) execute hd_video_record_with_2proc [enc_type], it will start with above data flow.
	(4) press 'q' to quit this app.
	(5) enter "sync" to ensure all files are saved.
	(6) generate output files: check it on /mnt/sd/dump_bs_main.dat.
	NOTE: [enc_type] 0:H265 1:H264 2:JPEG

hd_video_record_with_dir:
	[data flow] videocap -> videoproc -> videoenc -> (user)
	[usage]
	(1) require input files: (none).
	(2) require sensor board imx291.
	(3) execute hd_video_record_with_dir [enc_type], it will start with above data flow.
	(4) press 'q' to quit this app.
        	press 'w or x' to dynamic changevideocap output mirror/flip (only support CCIR sensor)
        	press 'e or c' to dynamic changevideoproc input mirror/flip
	(5) enter "sync" to ensure all files are saved.
	(6) generate output files: check it on /mnt/sd/dump_bs_main.dat.
	NOTE: [enc_type] 0:H265 1:H264 2:JPEG

hd_video_record_with_extend:
	[data flow] videocap -> videoproc -> videoenc -> (user)
					  \> videoenc -> (user)
	[usage]
	(1) require input files: (none).
	(2) require sensor board imx291.
	(3) execute hd_video_record_with_extend [enc_type] [extend mode], it will start with above data flow.
	(4) press 'q' to quit this app.
	(5) enter "sync" to ensure all files are saved.
	(6) generate output files: check it on /mnt/sd/dump_bs_main.dat and /mnt/sd/dump_bs_sub.dat.
	NOTE: [enc_type] 0:H265 1:H264 2:JPEG
	NOTE: [extend mode] 0:share 1:scale 2:rotate 2:scale+rotate

hd_video_record_with_frc:
	[data flow] videocap -> videoproc -> videoenc -> (user)
	[usage]
	(1) require input files: (none).
	(2) require sensor board imx291.
	(3) execute hd_video_record_with_frc [enc_type], it will start with above data flow.
	(4) press 'q' to quit this app.
		press 'e or c' to dynamic change videoproc input frame rate
		press 'r or v' to dynamic change videoproc output frame rate
	(5) enter "sync" to ensure all files are saved.
	(6) generate output files: check it on /mnt/sd/dump_bs_main.dat.
	NOTE: [enc_type] 0:H265 1:H264 2:JPEG

hd_video_record_with_osg:
	[data flow] videocap -> videoproc -> videoenc -> (user)
	[usage]
	(1) require input files: /pattern/video_frm_1000_200_1_argb4444.dat, copy it to /mnt/sd/video_frm_1000_200_1_argb4444.dat.
	(2) execute hd_video_record_with_osg, it will start to record streaming.
	(3) press 'q' to quit this app.
	(4) enter "sync" to ensure the output file is fully saved.
	(5) generated output files: /mnt/sd/dump_bs_main.dat.
	(6) play dump_bs_main.dat. Two blue "novatek" logos and a red mask are on the top left.

hd_video_record_with_substream:
	[data flow] videocap -> videoproc -> videoenc -> (user)
					  \> videoenc -> (user)
	[usage]
	(1) require input files: (none).
	(2) require sensor board imx291.
	(3) execute hd_video_record_with_substream [enc_type], it will start with above data flow.
	(4) press 'q' to quit this app.
	(5) enter "sync" to ensure all files are saved.
	(6) generate output files: check it on /mnt/sd/dump_bs_main.dat and /mnt/sd/dump_bs_sub.dat.
	NOTE: [enc_type] 0:H265 1:H264 2:JPEG

hd_video_record_with_userqp:
	[data flow] videocap -> videoproc -> videoenc -> (user)
	[usage]
	(1) require input files: (none).
	(2) require sensor board imx291.
	(3) execute hd_video_record_with_userqp, it will start with above data flow.
	(4) press 'q' to quit this app.
	(5) enter "sync" to ensure all files are saved.
	(6) generate output files: check it on /mnt/sd/dump_bs_main.dat.

hd_video_snapshot:
	[data flow] videocap -> videoproc -> (user) -> videoenc -> (user)
	[usage]
	(1) require input files: (none).
	(2) require sensor board imx291.
	(3) execute hd_video_snapshot, it will start with above data flow.
	(4) press 's' to trigger snapshot, it will save a snapshot file.
	(5) press 'q' to quit this app.
	(6) enter "sync" to ensure all files are saved.
	(7) generate output files: check it on /mnt/sd/dump_bs_snapxxx.jpg.

hd_video_snapshot_with_extend:
	[data flow] videocap -> videoproc ->
					  \> (user) -> videoenc -> (user)
	[usage]
	(1) require input files: (none).
	(2) require sensor board imx291.
	(3) execute hd_video_snapshot_with_extend [extend mode], it will start with above data flow.
	(4) press 's' to trigger snapshot, it will save a snapshot file.
	(5) press 'q' to quit this app.
	(6) enter "sync" to ensure all files are saved.
	(7) generate output files: check it on /mnt/sd/dump_bs_snapxxx_[extend mode].jpg.
	NOTE: [extend mode] 0:share 1:scale 2:rotate 2:scale+rotate

hd_video_snapshot_with_osg:
	[data flow] videocap -> videoproc -> videoenc
                                              \-> (user)
	[usage]
	(1) require input files: (none).
	(2) require sensor board imx291.
	(3) execute hd_video_snapshot_with_osg, it will start with above data flow.
	(4) press 's' to trigger snapshot, it will save a snapshot file.
	(5) press 'q' to quit this app.
	(6) enter "sync" to ensure all files are saved.
	(7) generate output files: check it on /mnt/sd/dump_bs_snapxxx.jpg.

hd_video_record_with_pano:
	[data flow] videocap -> videoproc -> videoenc -> (user)
	[usage]
	(1) require input files: (none).
	(2) require sensor board imx291.
	(3) execute hd_video_record [enc_type], it will start panorama 360 effect with above data flow.
	(4) press 'q' to quit this app.
	(5) enter "sync" to ensure all files are saved.
	(6) generate output files: check it on /mnt/sd/dump_bs_main.dat.
	NOTE: [enc_type] 0:H265 1:H264 2:JPEG
