/* SPDX-License-Identifier: GPL-2.0 */
/*
 * compat.h - incluido a forca (-include) antes de todo arquivo do
 * fabricante. Reconstroi, por cima do kernel novo, a API que o driver
 * de 2015 espera. A regra: so traduzir nomes e assinaturas, sem mudar
 * o que o driver faz. O que nao cabe aqui vira patch em patches/.
 */
#ifndef SSV_COMPAT_H
#define SSV_COMPAT_H

#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/fs.h>
#include <linux/time64.h>
#include <linux/timekeeping.h>
#include <linux/timer.h>
#include <linux/notifier.h>
#include <linux/cpu.h>
#include <linux/scatterlist.h>
#include <linux/platform_device.h>
#include <net/mac80211.h>

/* ---- modulo ------------------------------------------------------- */
#define MODULE_SUPPORTED_DEVICE(x)

/* ---- strings: strncpy saiu do kernel ------------------------------ */
#ifndef strncpy
static inline char *ssv_strncpy(char *dst, const char *src, size_t n)
{
	size_t i;

	for (i = 0; i < n && src[i]; i++)
		dst[i] = src[i];
	for (; i < n; i++)
		dst[i] = '\0';
	return dst;
}
#define strncpy ssv_strncpy
#endif

/* ---- tempo: timespec virou timespec64 ----------------------------- */
#define timespec timespec64
#define getnstimeofday(ts) ktime_get_real_ts64(ts)
#define timespec_sub(a, b) timespec64_sub(a, b)
#define timespec_to_ns(ts) timespec64_to_ns(ts)

/* ---- timers -------------------------------------------------------- */
#define del_timer_sync(t) timer_delete_sync(t)
#define del_timer(t) timer_delete(t)

/* ---- acesso a arquivo: get_fs/set_fs sumiram ---------------------- */
/* kernel_read/kernel_write ja leem para buffer do kernel; o par
 * get_fs/set_fs vira enfeite. */
typedef int mm_segment_t;
#define KERNEL_DS 0
#define get_ds() 0
#define get_fs() 0
#define set_fs(x) do { (void)(x); } while (0)
/* assinatura antiga: kernel_read(file, offset, buf, count) */
static inline int ssv_kernel_read_old(struct file *f, loff_t off, char *buf,
				      unsigned long count)
{
	return kernel_read(f, buf, count, &off);
}
#define kernel_read(f, off, buf, cnt) ssv_kernel_read_old(f, off, buf, cnt)
/* fp->f_op->read/write nao existem mais na maioria dos sistemas de
 * arquivos (so read_iter/write_iter); o patch 0009 chama estes */
#define ssv_file_read(f, buf, n, pos) (kernel_read)(f, buf, n, pos)
#define ssv_file_write(f, buf, n, pos) kernel_write(f, buf, n, pos)
/* vfs_read deixou de ser exportado; com buffer do kernel e o kernel_read */
#define vfs_read(f, buf, n, pos) (kernel_read)(f, buf, n, pos)

/* ---- notificador de CPU (trocado por cpuhp) ----------------------- */
#define CPU_ONLINE		0x0002
#define CPU_UP_PREPARE		0x0003
#define CPU_UP_CANCELED		0x0004
#define CPU_DOWN_PREPARE	0x0005
#define CPU_DOWN_FAILED		0x0006
#define CPU_DEAD		0x0007
#define CPU_TASKS_FROZEN	0x0010
#define CPU_ONLINE_FROZEN	(CPU_ONLINE | CPU_TASKS_FROZEN)
#define CPU_UP_PREPARE_FROZEN	(CPU_UP_PREPARE | CPU_TASKS_FROZEN)
#define CPU_UP_CANCELED_FROZEN	(CPU_UP_CANCELED | CPU_TASKS_FROZEN)
#define CPU_DOWN_PREPARE_FROZEN	(CPU_DOWN_PREPARE | CPU_TASKS_FROZEN)
#define CPU_DOWN_FAILED_FROZEN	(CPU_DOWN_FAILED | CPU_TASKS_FROZEN)
#define CPU_DEAD_FROZEN		(CPU_DEAD | CPU_TASKS_FROZEN)
int ssv_register_cpu_notifier(struct notifier_block *nb);
void ssv_unregister_cpu_notifier(struct notifier_block *nb);
#define register_cpu_notifier(nb) ssv_register_cpu_notifier(nb)
#define unregister_cpu_notifier(nb) ssv_unregister_cpu_notifier(nb)

/* ---- crypto: blkcipher/hash/cipher saíram da API publica ---------- */
/* Reimplementados sobre a biblioteca do kernel (lib/crypto: arc4, aes)
 * e um Michael MIC local. So o que o driver usa: ecb(arc4),
 * michael_mic e aes de um bloco. */
#include <crypto/arc4.h>
#include <crypto/aes.h>
/* incluidos antes das macros, para os headers do kernel nao serem
 * reprocessados depois com os nomes trocados */
#include <crypto/skcipher.h>
#include <crypto/hash.h>
#include <linux/crypto.h>

struct crypto_blkcipher {
	struct arc4_ctx ctx;
};
struct blkcipher_desc {
	struct crypto_blkcipher *tfm;
	u32 flags;
};
struct crypto_hash {
	u8 key[8];
};
struct hash_desc {
	struct crypto_hash *tfm;
	u32 flags;
};
#define crypto_cipher ssv_crypto_cipher
struct ssv_crypto_cipher_alg {
	const char *cra_name, *cra_driver_name;
	struct module *cra_module;
};
struct ssv_crypto_cipher {
	struct aes_enckey key;
	/* o driver imprime tfm->base.__crt_alg->cra_name */
	struct { const struct ssv_crypto_cipher_alg *__crt_alg; } base;
};

struct crypto_blkcipher *ssv_crypto_alloc_blkcipher(const char *name);
struct crypto_hash *ssv_crypto_alloc_hash(const char *name);
struct ssv_crypto_cipher *ssv_crypto_alloc_cipher(const char *name);
#define crypto_alloc_blkcipher(n, t, m) ssv_crypto_alloc_blkcipher(n)
#define crypto_alloc_hash(n, t, m) ssv_crypto_alloc_hash(n)
#define crypto_alloc_cipher(n, t, m) ssv_crypto_alloc_cipher(n)
#define crypto_free_blkcipher(t) kfree_sensitive(t)
#define crypto_free_hash(t) kfree_sensitive(t)
#define crypto_free_cipher(t) kfree_sensitive(t)

int ssv_crypto_blkcipher_setkey(struct crypto_blkcipher *t, const u8 *k,
				unsigned int len);
int ssv_crypto_blkcipher_crypt(struct blkcipher_desc *d,
			       struct scatterlist *dst,
			       struct scatterlist *src, unsigned int len);
#define crypto_blkcipher_setkey ssv_crypto_blkcipher_setkey
#define crypto_blkcipher_encrypt ssv_crypto_blkcipher_crypt
#define crypto_blkcipher_decrypt ssv_crypto_blkcipher_crypt

int ssv_crypto_hash_setkey(struct crypto_hash *t, const u8 *k,
			   unsigned int len);
int ssv_crypto_hash_digest(struct hash_desc *d, struct scatterlist *sg,
			   unsigned int len, u8 *out);
#define crypto_hash_setkey ssv_crypto_hash_setkey
#define crypto_hash_digest ssv_crypto_hash_digest

int ssv_crypto_cipher_setkey(struct ssv_crypto_cipher *t, const u8 *k,
			     unsigned int len);
void ssv_crypto_cipher_encrypt_one(struct ssv_crypto_cipher *t, u8 *dst,
				   const u8 *src);
#define crypto_cipher_setkey ssv_crypto_cipher_setkey
#define crypto_cipher_encrypt_one ssv_crypto_cipher_encrypt_one

/* skcipher/ahash (caminho >= 4.6 do driver): "ecb(arc4)" ate existe no
 * kernel, mas so com CRYPTO_USER_API_ENABLE_OBSOLETE, e "michael_mic" nao
 * existe mais como hash. Usam a mesma implementacao acima. */
#define crypto_skcipher crypto_blkcipher
#define crypto_ahash crypto_hash
#define crypto_alloc_skcipher(n, t, m) ssv_crypto_alloc_blkcipher(n)
#define crypto_alloc_ahash(n, t, m) ssv_crypto_alloc_hash(n)
#define crypto_free_skcipher(t) kfree_sensitive(t)
#define crypto_free_ahash(t) kfree_sensitive(t)
#define crypto_skcipher_setkey ssv_crypto_blkcipher_setkey
#define crypto_ahash_setkey ssv_crypto_hash_setkey

struct ssv_skcipher_req {
	struct crypto_blkcipher *tfm;
	struct scatterlist *src, *dst;
	unsigned int len;
};
#define skcipher_request ssv_skcipher_req
#define SKCIPHER_REQUEST_ON_STACK(name, t) \
	struct ssv_skcipher_req __##name##_d = { .tfm = (t) }, *name = &__##name##_d
#define skcipher_request_set_tfm(r, t) ((r)->tfm = (t))
#define skcipher_request_set_callback(r, f, c, d) do { } while (0)
#define skcipher_request_set_crypt(r, s, d, l, iv) \
	((r)->src = (s), (r)->dst = (d), (r)->len = (l))
#define skcipher_request_zero(r) memzero_explicit(r, sizeof(*(r)))
static inline int ssv_skcipher_crypt(struct ssv_skcipher_req *r)
{
	struct blkcipher_desc d = { .tfm = r->tfm };

	return ssv_crypto_blkcipher_crypt(&d, r->dst, r->src, r->len);
}
#define crypto_skcipher_encrypt(r) ssv_skcipher_crypt(r)
#define crypto_skcipher_decrypt(r) ssv_skcipher_crypt(r)

struct ssv_ahash_req {
	struct crypto_hash *tfm;
	struct scatterlist *sg;
	u8 *out;
	unsigned int len;
};
#define ahash_request ssv_ahash_req
#define AHASH_REQUEST_ON_STACK(name, t) \
	struct ssv_ahash_req __##name##_d = { .tfm = (t) }, *name = &__##name##_d
#define ahash_request_set_tfm(r, t) ((r)->tfm = (t))
#define ahash_request_set_callback(r, f, c, d) do { } while (0)
#define ahash_request_set_crypt(r, s, o, l) \
	((r)->sg = (s), (r)->out = (o), (r)->len = (l))
#define ahash_request_zero(r) memzero_explicit(r, sizeof(*(r)))
static inline int ssv_ahash_digest(struct ssv_ahash_req *r)
{
	struct hash_desc d = { .tfm = r->tfm };

	return ssv_crypto_hash_digest(&d, r->sg, r->len, r->out);
}
#define crypto_ahash_digest(r) ssv_ahash_digest(r)

/* ---- tempo (2) ----------------------------------------------------- */
#include <linux/sched/clock.h>
#define cpu_clock(cpu) local_clock()
#define get_monotonic_boottime(ts) ktime_get_boottime_ts64(ts)
#define PDE_DATA(inode) pde_data(inode)

/* ---- mac80211/cfg80211: nomes ------------------------------------- */
#define ieee80211_band nl80211_band
#define IEEE80211_BAND_2GHZ NL80211_BAND_2GHZ
#define IEEE80211_BAND_5GHZ NL80211_BAND_5GHZ
#define IEEE80211_NUM_BANDS NUM_NL80211_BANDS
#define IEEE80211_MAX_AMPDU_BUF IEEE80211_MAX_AMPDU_BUF_HT
#define FIF_PROMISC_IN_BSS 0
#define ieee80211_tx_status(hw, skb) ieee80211_tx_status_skb(hw, skb)


/* ---- diversos ----------------------------------------------------- */
#define cpus_allowed cpus_mask
/* caminho so para controladora SDHCI em PCI (notebook x86); as TV boxes
 * nao tem, e o sdhci.h do kernel deixou de ser publico */
#undef CONFIG_PCIEASPM
/* cfg80211 exporta um michael_mic() proprio; o do driver e outro */
#define michael_mic ssv_michael_mic
/* parametro novo (link_id); o driver so tem um link, o 0 */
#define ieee80211_beacon_get_tim(hw, vif, off, len) \
	ieee80211_beacon_get_tim(hw, vif, off, len, 0)


/* sched_setscheduler deixou de ser exportado; sched_setattr_nocheck faz o
 * mesmo (sem checagem de permissao, como era a chamada de dentro do
 * kernel) */
#include <linux/sched.h>
#include <uapi/linux/sched/types.h>
static inline int ssv_sched_setscheduler(struct task_struct *p, int policy,
					 const struct sched_param *sp)
{
	struct sched_attr a = {
		.size = sizeof(a),
		.sched_policy = policy,
		.sched_priority = sp->sched_priority,
	};

	return sched_setattr_nocheck(p, &a);
}
#define sched_setscheduler(p, pol, sp) ssv_sched_setscheduler(p, pol, sp)

/* procfs: proc_create agora recebe proc_ops */
#include <linux/proc_fs.h>
struct proc_dir_entry *ssv_proc_create(const char *name, umode_t mode,
				       struct proc_dir_entry *parent,
				       const struct file_operations *fops);
#define proc_create(n, m, p, f) ssv_proc_create(n, m, p, f)
struct proc_dir_entry *ssv_proc_create_data(const char *name, umode_t mode,
					    struct proc_dir_entry *parent,
					    const struct file_operations *fops,
					    void *data);
#define proc_create_data(n, m, p, f, d) ssv_proc_create_data(n, m, p, f, d)

/* ---- controle de taxa: alloc perdeu o parametro de debugfs -------- */
/* rate_control_send_low: o mac80211 ja faz isso antes de chamar
 * get_rate (rate_control_get_rate), entao para o driver e sempre 0 */
#define rate_control_send_low(sta, priv_sta, txrc) false
struct ssv_legacy_rc_ops {
	unsigned long capa;
	struct module *module;
	const char *name;
	void *(*alloc)(struct ieee80211_hw *hw, struct dentry *debugfsdir);
	void (*free)(void *priv);
	void *(*alloc_sta)(void *priv, struct ieee80211_sta *sta, gfp_t gfp);
	void (*rate_init)(void *priv, struct ieee80211_supported_band *sband,
			  struct cfg80211_chan_def *chandef,
			  struct ieee80211_sta *sta, void *priv_sta);
	void (*rate_update)(void *priv, struct ieee80211_supported_band *sband,
			    struct cfg80211_chan_def *chandef,
			    struct ieee80211_sta *sta, void *priv_sta,
			    u32 changed);
	void (*free_sta)(void *priv, struct ieee80211_sta *sta,
			 void *priv_sta);
	void (*tx_status)(void *priv, struct ieee80211_supported_band *sband,
			  struct ieee80211_sta *sta, void *priv_sta,
			  struct sk_buff *skb);
	void (*get_rate)(void *priv, struct ieee80211_sta *sta, void *priv_sta,
			 struct ieee80211_tx_rate_control *txrc);
	void (*add_sta_debugfs)(void *priv, void *priv_sta,
				struct dentry *dir);
};
int ssv_rate_control_register(const struct ssv_legacy_rc_ops *ops);
void ssv_rate_control_unregister(const struct ssv_legacy_rc_ops *ops);

/* ---- mac80211: callbacks com a assinatura que o driver conhece ---- */
/* O driver preenche esta tabela (com o nome "ieee80211_ops" trocado
 * por macro) e compat.c monta a tabela real com funcoes-ponte. */
struct ssv_legacy_ops {
	void (*tx)(struct ieee80211_hw *hw,
		   struct ieee80211_tx_control *control, struct sk_buff *skb);
	int (*start)(struct ieee80211_hw *hw);
	void (*stop)(struct ieee80211_hw *hw);
	int (*add_interface)(struct ieee80211_hw *hw,
			     struct ieee80211_vif *vif);
	void (*remove_interface)(struct ieee80211_hw *hw,
				 struct ieee80211_vif *vif);
	int (*config)(struct ieee80211_hw *hw, u32 changed);
	void (*configure_filter)(struct ieee80211_hw *hw,
				 unsigned int changed_flags,
				 unsigned int *total_flags, u64 multicast);
	void (*bss_info_changed)(struct ieee80211_hw *hw,
				 struct ieee80211_vif *vif,
				 struct ieee80211_bss_conf *info, u32 changed);
	int (*sta_add)(struct ieee80211_hw *hw, struct ieee80211_vif *vif,
		       struct ieee80211_sta *sta);
	int (*sta_remove)(struct ieee80211_hw *hw, struct ieee80211_vif *vif,
			  struct ieee80211_sta *sta);
	void (*sta_notify)(struct ieee80211_hw *hw, struct ieee80211_vif *vif,
			   enum sta_notify_cmd cmd, struct ieee80211_sta *sta);
	int (*set_key)(struct ieee80211_hw *hw, enum set_key_cmd cmd,
		       struct ieee80211_vif *vif, struct ieee80211_sta *sta,
		       struct ieee80211_key_conf *key);
	void (*sw_scan_start)(struct ieee80211_hw *hw,
			      struct ieee80211_vif *vif, const u8 *mac_addr);
	void (*sw_scan_complete)(struct ieee80211_hw *hw,
				 struct ieee80211_vif *vif);
	u64 (*get_tsf)(struct ieee80211_hw *hw, struct ieee80211_vif *vif);
	int (*set_tim)(struct ieee80211_hw *hw, struct ieee80211_sta *sta,
		       bool set);
	int (*conf_tx)(struct ieee80211_hw *hw, struct ieee80211_vif *vif,
		       u16 queue, const struct ieee80211_tx_queue_params *p);
	/* este driver ja usa a forma nova (>= 4.4) */
	int (*ampdu_action)(struct ieee80211_hw *hw, struct ieee80211_vif *vif,
			    struct ieee80211_ampdu_params *params);
	int (*change_interface)(struct ieee80211_hw *hw,
				struct ieee80211_vif *vif,
				enum nl80211_iftype new_type, bool p2p);
	void (*set_default_unicast_key)(struct ieee80211_hw *hw,
					struct ieee80211_vif *vif, int idx);
	int (*suspend)(struct ieee80211_hw *hw, struct cfg80211_wowlan *w);
	int (*resume)(struct ieee80211_hw *hw);
};

struct ieee80211_hw *ssv_compat_alloc_hw(size_t priv_len,
					 const struct ssv_legacy_ops *ops);

#ifndef SSV_COMPAT_IMPL
#define ieee80211_ops ssv_legacy_ops
#define ieee80211_alloc_hw(len, ops) ssv_compat_alloc_hw(len, ops)
#define rate_control_ops ssv_legacy_rc_ops
#define ieee80211_rate_control_register(o) ssv_rate_control_register(o)
#define ieee80211_rate_control_unregister(o) ssv_rate_control_unregister(o)
#endif

#endif /* SSV_COMPAT_H */
