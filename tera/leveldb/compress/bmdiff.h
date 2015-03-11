// Copyright (c) 2015, Baidu.com, Inc. All Rights Reserved
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LEVELDB_COMPRESS_BM_DIFF_H
#define LEVELDB_COMPRESS_BM_DIFF_H

#define BM_E_OK    0
#define BM_E_ERROR           (-1)
#define BM_E_INPUT_OVERRUN   (-4)
#define BM_E_OUTPUT_OVERRUN  (-5)
#define BM_E_ERROR_FORMAT    (-6)

namespace bmz {

/**
 * @brief ����bm��������Ҫ�ĸ����ڴ��С
 *
 * @param in_len ���볤��
 * @param fp_len ̽�볤��
 *
 * @return �����ڴ��С
 */
size_t  bm_encode_worklen(size_t in_len, size_t fp_len);

/**
 * @brief ����bm��������Ҫ�����ڴ��С
 *
 * @param out_len �������
 *
 * @return �����ڴ��С
 */
size_t bm_decode_worklen(size_t out_len);


/**
 * @brief bmѹ������
 *
 * @param src Դ��ַ
 * @param in_len Դ����
 * @param dst Ŀ���ַ
 * @param out_len_p �洢Ŀ�곤�ȵ�������ַ���ɹ��󳤶�ֵ�޸�Ϊʵ�ʵ�ѹ���󳤶�
 * @param fp_len ̽�볤�ȣ�����Ϊ[fp_len, 2*fp_len]֮��Ĺ��������ֱ�ѹ�������ȴ���2*fp_len��ض���ѹ��
 * @param work_mem �����ڴ棬���ڴ洢HashTable.�䳤������fp_len��in_len
 *
 * @return �ɹ�����BMZ_E_OK,ʧ�ܷ�����Ӧ�Ĵ�����
 */

int bm_encode_body(const void *src, size_t in_len,
                   void *dst, size_t *out_len_p,
                   size_t fp_len,  void *work_mem);

/**
 * @brief bm��ѹ����
 *
 * @param src Դ��ַ
 * @param in_len Դ����
 * @param dst Ŀ���ַ
 * @param out_len_p Ŀ�곤��ָ��,ֻ�е�Ŀ�곤�ȴ���Դ����+1ʱ�����ȫ
 *
 * @return �ɹ��󷵻�BMZ_E_OK,ʧ�ܷ�����Ӧ�Ĵ�����
 */

int bm_decode_body(const void *src, size_t in_len,
                   void *dst, size_t *out_len_p);

} // namespace bmz

#endif // LEVELDB_COMPRESS_BM_DIFF_H
