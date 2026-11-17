// SPDX-License-Identifier: GPL-2.0
/*
 * compat.c - a parte da traducao que nao cabe em macro:
 *   - tabela de callbacks do mac80211 (assinaturas antigas -> novas);
 *   - notificador de CPU sobre cpuhp;
 *   - blkcipher/hash/cipher sobre lib/crypto.
 */
#ifndef SSV_COMPAT_IMPL
#error "compat.c precisa de -DSSV_COMPAT_IMPL (ver Kbuild)"
#endif
#include "compat.h"
#include <linux/cpuhotplug.h>
#include <linux/unaligned.h>

/* ==== mac80211 ====================================================== */

/* Um aparelho por vez: o driver original tambem so suporta um. */
static const struct ssv_legacy_ops *legacy;
static struct ieee80211_ops real_ops;

static void br_stop(struct ieee80211_hw *hw, bool suspend)
{
	legacy->stop(hw);
}

static int br_config(struct ieee80211_hw *hw, int radio_idx, u32 changed)
{
	return legacy->config(hw, changed);
}

static void br_bss_info_changed(struct ieee80211_hw *hw,
				struct ieee80211_vif *vif,
				struct ieee80211_bss_conf *info, u64 changed)
{
	legacy->bss_info_changed(hw, vif, info, (u32)changed);
}

static int br_conf_tx(struct ieee80211_hw *hw, struct ieee80211_vif *vif,
		      unsigned int link_id, u16 ac,
		      const struct ieee80211_tx_queue_params *p)
{
	return legacy->conf_tx(hw, vif, ac, p);
}

struct ieee80211_hw *ssv_compat_alloc_hw(size_t priv_len,
					 const struct ssv_legacy_ops *ops)
{
	legacy = ops;
	memset(&real_ops, 0, sizeof(real_ops));

	/* mesma assinatura: passa direto */
	real_ops.tx = ops->tx;
	real_ops.start = ops->start;
	real_ops.add_interface = ops->add_interface;
	real_ops.remove_interface = ops->remove_interface;
	real_ops.configure_filter = ops->configure_filter;
	real_ops.sta_add = ops->sta_add;
	real_ops.sta_remove = ops->sta_remove;
	real_ops.sta_notify = ops->sta_notify;
	real_ops.set_key = ops->set_key;
	real_ops.sw_scan_start = ops->sw_scan_start;
	real_ops.sw_scan_complete = ops->sw_scan_complete;
	real_ops.get_tsf = ops->get_tsf;
	real_ops.set_tim = ops->set_tim;
	real_ops.ampdu_action = ops->ampdu_action;
	real_ops.change_interface = ops->change_interface;
	real_ops.set_default_unicast_key = ops->set_default_unicast_key;
#ifdef CONFIG_PM
	real_ops.suspend = ops->suspend;
	real_ops.resume = ops->resume;
#endif

	/* assinatura mudou: funcao-ponte */
	real_ops.stop = br_stop;
	real_ops.config = br_config;
	if (ops->bss_info_changed)
		real_ops.bss_info_changed = br_bss_info_changed;
	if (ops->conf_tx)
		real_ops.conf_tx = br_conf_tx;

	/* exigidos hoje, inexistentes em 2015: os genericos do mac80211 */
	real_ops.wake_tx_queue = ieee80211_handle_wake_tx_queue;
	real_ops.add_chanctx = ieee80211_emulate_add_chanctx;
	real_ops.remove_chanctx = ieee80211_emulate_remove_chanctx;
	real_ops.change_chanctx = ieee80211_emulate_change_chanctx;
	real_ops.switch_vif_chanctx = ieee80211_emulate_switch_vif_chanctx;

	return ieee80211_alloc_hw(priv_len, &real_ops);
}

/* ==== controle de taxa ============================================= */

static const struct ssv_legacy_rc_ops *legacy_rc;
static struct rate_control_ops real_rc;

static void *br_rc_alloc(struct ieee80211_hw *hw)
{
	return legacy_rc->alloc(hw, NULL);
}

int ssv_rate_control_register(const struct ssv_legacy_rc_ops *ops)
{
	legacy_rc = ops;
	memset(&real_rc, 0, sizeof(real_rc));
	real_rc.capa = ops->capa;
	real_rc.name = ops->name;
	real_rc.alloc = br_rc_alloc;
	real_rc.free = ops->free;
	real_rc.alloc_sta = ops->alloc_sta;
	real_rc.rate_init = ops->rate_init;
	real_rc.rate_update = ops->rate_update;
	real_rc.free_sta = ops->free_sta;
	real_rc.tx_status = ops->tx_status;
	real_rc.get_rate = ops->get_rate;
	real_rc.add_sta_debugfs = ops->add_sta_debugfs;
	return ieee80211_rate_control_register(&real_rc);
}

void ssv_rate_control_unregister(const struct ssv_legacy_rc_ops *ops)
{
	ieee80211_rate_control_unregister(&real_rc);
}

/* ==== procfs ======================================================== */
/* proc_create recebe proc_ops, e as funcoes de proc_ops nao recebem
 * nenhum ponteiro de volta para o file_operations original. Entao cada
 * file_operations do driver ganha um "slot" com funcoes-ponte proprias. */

#define SSV_PROC_SLOTS 8
static const struct file_operations *proc_fops[SSV_PROC_SLOTS];

#define SSV_PROC_SLOT(i)						\
static int br_proc_open##i(struct inode *in, struct file *f)		\
{									\
	return proc_fops[i]->open ? proc_fops[i]->open(in, f) : 0;	\
}									\
static ssize_t br_proc_read##i(struct file *f, char __user *b,		\
			       size_t n, loff_t *p)			\
{									\
	return proc_fops[i]->read ? proc_fops[i]->read(f, b, n, p)	\
				  : -EINVAL;				\
}									\
static ssize_t br_proc_write##i(struct file *f, const char __user *b,	\
				size_t n, loff_t *p)			\
{									\
	return proc_fops[i]->write ? proc_fops[i]->write(f, b, n, p)	\
				   : -EINVAL;				\
}									\
static int br_proc_release##i(struct inode *in, struct file *f)	\
{									\
	return proc_fops[i]->release ? proc_fops[i]->release(in, f) : 0;\
}

SSV_PROC_SLOT(0) SSV_PROC_SLOT(1) SSV_PROC_SLOT(2) SSV_PROC_SLOT(3)
SSV_PROC_SLOT(4) SSV_PROC_SLOT(5) SSV_PROC_SLOT(6) SSV_PROC_SLOT(7)

#define SSV_PROC_OPS(i) {						\
	.proc_open = br_proc_open##i, .proc_read = br_proc_read##i,	\
	.proc_write = br_proc_write##i,					\
	.proc_release = br_proc_release##i,				\
	.proc_lseek = default_llseek }
static const struct proc_ops br_proc_ops[SSV_PROC_SLOTS] = {
	SSV_PROC_OPS(0), SSV_PROC_OPS(1), SSV_PROC_OPS(2), SSV_PROC_OPS(3),
	SSV_PROC_OPS(4), SSV_PROC_OPS(5), SSV_PROC_OPS(6), SSV_PROC_OPS(7),
};

static const struct proc_ops *ssv_proc_slot(const struct file_operations *f)
{
	int i;

	for (i = 0; i < SSV_PROC_SLOTS; i++) {
		if (proc_fops[i] == f || !proc_fops[i]) {
			proc_fops[i] = f;
			return &br_proc_ops[i];
		}
	}
	return NULL;
}

struct proc_dir_entry *ssv_proc_create_data(const char *name, umode_t mode,
					    struct proc_dir_entry *parent,
					    const struct file_operations *fops,
					    void *data)
{
	const struct proc_ops *ops = ssv_proc_slot(fops);

	if (WARN_ON(!ops))
		return NULL;
	return (proc_create_data)(name, mode, parent, ops, data);
}

struct proc_dir_entry *ssv_proc_create(const char *name, umode_t mode,
				       struct proc_dir_entry *parent,
				       const struct file_operations *fops)
{
	return ssv_proc_create_data(name, mode, parent, fops, NULL);
}

/* ==== notificador de CPU =========================================== */

static struct notifier_block *cpu_nb;
static enum cpuhp_state cpu_state;

static int ssv_cpu_online(unsigned int cpu)
{
	cpu_nb->notifier_call(cpu_nb, CPU_UP_PREPARE, (void *)(long)cpu);
	cpu_nb->notifier_call(cpu_nb, CPU_ONLINE, (void *)(long)cpu);
	return 0;
}

static int ssv_cpu_offline(unsigned int cpu)
{
	cpu_nb->notifier_call(cpu_nb, CPU_DOWN_PREPARE, (void *)(long)cpu);
	return 0;
}

int ssv_register_cpu_notifier(struct notifier_block *nb)
{
	int ret;

	cpu_nb = nb;
	/* _nocalls: o notificador antigo nao era chamado no registro */
	ret = cpuhp_setup_state_nocalls(CPUHP_AP_ONLINE_DYN, "ssv6051:online",
					ssv_cpu_online, ssv_cpu_offline);
	if (ret < 0)
		return ret;
	cpu_state = ret;
	return 0;
}

void ssv_unregister_cpu_notifier(struct notifier_block *nb)
{
	if (cpu_state > 0)
		cpuhp_remove_state_nocalls(cpu_state);
	cpu_state = 0;
}

/* ==== crypto ======================================================== */

struct crypto_blkcipher *ssv_crypto_alloc_blkcipher(const char *name)
{
	struct crypto_blkcipher *t;

	if (strcmp(name, "ecb(arc4)"))
		return ERR_PTR(-ENOENT);
	t = kzalloc(sizeof(*t), GFP_ATOMIC);
	return t ? t : ERR_PTR(-ENOMEM);
}

int ssv_crypto_blkcipher_setkey(struct crypto_blkcipher *t, const u8 *k,
				unsigned int len)
{
	return arc4_setkey(&t->ctx, k, len);
}

/* O driver so chama com um sg linear e in-place, mas percorre a lista
 * por garantia. ARC4 e simetrico: cifrar e decifrar sao o mesmo. */
int ssv_crypto_blkcipher_crypt(struct blkcipher_desc *d,
			       struct scatterlist *dst,
			       struct scatterlist *src, unsigned int len)
{
	struct scatterlist *s = src, *o = dst;

	while (len && s && o) {
		unsigned int n = min(len, min(s->length, o->length));

		arc4_crypt(&d->tfm->ctx, sg_virt(o), sg_virt(s), n);
		len -= n;
		s = sg_next(s);
		o = sg_next(o);
	}
	return len ? -EINVAL : 0;
}

struct crypto_hash *ssv_crypto_alloc_hash(const char *name)
{
	struct crypto_hash *t;

	if (strcmp(name, "michael_mic"))
		return ERR_PTR(-ENOENT);
	t = kzalloc(sizeof(*t), GFP_ATOMIC);
	return t ? t : ERR_PTR(-ENOMEM);
}

int ssv_crypto_hash_setkey(struct crypto_hash *t, const u8 *k,
			   unsigned int len)
{
	if (len != 8)
		return -EINVAL;
	memcpy(t->key, k, 8);
	return 0;
}

/* Michael MIC (IEEE 802.11 TKIP), igual a crypto/michael_mic.c antigo */
#define MICHAEL_BLOCK(l, r)				\
	do {						\
		r ^= rol32(l, 17); l += r;		\
		r ^= ((l & 0xff00ff00) >> 8) |		\
		     ((l & 0x00ff00ff) << 8); l += r;	\
		r ^= rol32(l, 3); l += r;		\
		r ^= ror32(l, 2); l += r;		\
	} while (0)

int ssv_crypto_hash_digest(struct hash_desc *d, struct scatterlist *sg,
			   unsigned int len, u8 *out)
{
	u32 l = get_unaligned_le32(d->tfm->key);
	u32 r = get_unaligned_le32(d->tfm->key + 4);
	u8 pend[4];
	unsigned int np = 0;

	for (; sg && len; sg = sg_next(sg)) {
		const u8 *p = sg_virt(sg);
		unsigned int n = min(len, sg->length);

		len -= n;
		while (n--) {
			pend[np++] = *p++;
			if (np == 4) {
				l ^= get_unaligned_le32(pend);
				MICHAEL_BLOCK(l, r);
				np = 0;
			}
		}
	}
	/* fim: 0x5a e zeros ate completar o bloco, mais um bloco zero */
	pend[np++] = 0x5a;
	while (np < 4)
		pend[np++] = 0;
	l ^= get_unaligned_le32(pend);
	MICHAEL_BLOCK(l, r);
	MICHAEL_BLOCK(l, r);
	put_unaligned_le32(l, out);
	put_unaligned_le32(r, out + 4);
	return 0;
}

static const struct ssv_crypto_cipher_alg ssv_aes_alg = {
	.cra_name = "aes",
	.cra_driver_name = "aes-lib",
	.cra_module = THIS_MODULE,
};

struct ssv_crypto_cipher *ssv_crypto_alloc_cipher(const char *name)
{
	struct ssv_crypto_cipher *t;

	if (strcmp(name, "aes"))
		return ERR_PTR(-ENOENT);
	t = kzalloc(sizeof(*t), GFP_ATOMIC);
	if (!t)
		return ERR_PTR(-ENOMEM);
	t->base.__crt_alg = &ssv_aes_alg;
	return t;
}

int ssv_crypto_cipher_setkey(struct ssv_crypto_cipher *t, const u8 *k,
			     unsigned int len)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(7, 0, 0)
	return aes_prepareenckey(&t->key, k, len);
#else
	return aes_expandkey(&t->key, k, len);
#endif
}

void ssv_crypto_cipher_encrypt_one(struct ssv_crypto_cipher *t, u8 *dst,
				   const u8 *src)
{
	aes_encrypt(&t->key, dst, src);
}
