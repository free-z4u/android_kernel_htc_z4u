#include <linux/Yushan_API.h>
#include <linux/types.h>
#include <linux/ioctl.h>

#define RAWCHIP_IOCTL_MAGIC 'g'

#define RAWCHIP_IOCTL_GET_INT \
	_IOR(RAWCHIP_IOCTL_MAGIC, 1, struct rawchip_stats_event_ctrl *)

#define RAWCHIP_IOCTL_GET_AF_STATUS \
	_IOR(RAWCHIP_IOCTL_MAGIC, 2, struct rawchip_stats_event_ctrl *)

#define RAWCHIP_IOCTL_UPDATE_AEC_AWB \
	_IOW(RAWCHIP_IOCTL_MAGIC, 3, struct rawchip_stats_event_ctrl *)

#define RAWCHIP_IOCTL_UPDATE_AF \
	_IOW(RAWCHIP_IOCTL_MAGIC, 4, struct rawchip_stats_event_ctrl *)

#define RAWCHIP_IOCTL_UPDATE_3A \
	_IOW(RAWCHIP_IOCTL_MAGIC, 5, struct rawchip_stats_event_ctrl *)
	
#define IOCTL_POWER_ON_RAWCHIP			       _IO(RAWCHIP_IOCTL_MAGIC,  6)
#define IOCTL_POWER_OFF_RAWCHIP			       _IO(RAWCHIP_IOCTL_MAGIC,  7)
#define IOCTL_INIT_RAWCHIP					       _IO(RAWCHIP_IOCTL_MAGIC,  8)
#define IOCTL_DUMP_YUSHAN_REGISTERS	       _IO(RAWCHIP_IOCTL_MAGIC,  9)
#define IOCTL_ENABLE_YUSHAN_TX	           _IO(RAWCHIP_IOCTL_MAGIC,  10)
#define IOCTL_DISABLE_YUSHAN_TX	           _IO(RAWCHIP_IOCTL_MAGIC,  11)
#define IOCTL_CHANGE_RESOLUTION_TO_4X      _IO(RAWCHIP_IOCTL_MAGIC,  12)
#define IOCTL_CHANGE_RESOLUTION_TO_NORMAL  _IO(RAWCHIP_IOCTL_MAGIC,  13)
#define IOCTL_CHANGE_RAWCHIP_LOG_LEVEL \
	_IOW(RAWCHIP_IOCTL_MAGIC, 14, struct rawchip_stats_event_ctrl *)

struct rawchip_stats_event_ctrl {
	uint32_t type;
	uint32_t timeout_ms;
	uint32_t length;
	uint32_t logLevel;
	void *data;
};