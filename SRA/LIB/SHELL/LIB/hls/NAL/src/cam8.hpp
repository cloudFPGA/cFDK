/*******************************************************************************
 * Copyright 2016 -- 2021 IBM Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *******************************************************************************/

/*****************************************************************************
 * @file       : cam8.hpp
 * @brief      : A Content Address Memory (CAM) for 8 entries.
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Abstraction Layer (NAL)
 * Language    : Vivado HLS
 * 
 * \ingroup NAL
 * \addtogroup NAL
 * \{
 *****************************************************************************/


#ifndef _NAL_CAM8_H_
#define _NAL_CAM8_H_

#include <stdio.h>
#include <string>
#include <stdint.h>

#ifndef _NAL_KVP_DEF_
#define _NAL_KVP_DEF_
template<typename K, typename V>
struct KeyValuePair {
  public:
    K   key;
    V   value;
    bool      valid;
    KeyValuePair() {
      key = 0x0;
      value = 0x0;
      valid = false;
    }
    KeyValuePair(K key, V value) :
      key(key), value(value), valid(true) {}
    KeyValuePair(K key, V value, bool valid) :
      key(key), value(value), valid(valid) {}
};
#endif

template<typename K, typename V>
struct Cam8 {
  protected:
    KeyValuePair<K,V> CamArray0;
    KeyValuePair<K,V> CamArray1;
    KeyValuePair<K,V> CamArray2;
    KeyValuePair<K,V> CamArray3;
    KeyValuePair<K,V> CamArray4;
    KeyValuePair<K,V> CamArray5;
    KeyValuePair<K,V> CamArray6;
    KeyValuePair<K,V> CamArray7;
  public:
    Cam8() {
#pragma HLS pipeline II=1
      CamArray0.valid = false;
      CamArray1.valid = false;
      CamArray2.valid = false;
      CamArray3.valid = false;
      CamArray4.valid = false;
      CamArray5.valid = false;
      CamArray6.valid = false;
      CamArray7.valid = false;
    }
    /*******************************************************************************
     * @brief Search the CAM array for a key.
     *
     * @param[in]  key   The key to lookup.
     * @param[out] value The value corresponding to that key.
     *
     * @return true if the the key was found.
     *******************************************************************************/
    bool lookup(K key, V &value)
    {
#pragma HLS pipeline II=1
#pragma HLS INLINE
      if ((CamArray0.key == key) && (CamArray0.valid == true)) {
        value = CamArray0.value;
        return true;
      }
      else if ((CamArray1.key == key) && (CamArray1.valid == true)) {
        value = CamArray1.value;
        return true;
      }
      else if ((CamArray2.key == key) && (CamArray2.valid == true)) {
        value = CamArray2.value;
        return true;
      }
      else if ((CamArray3.key == key) && (CamArray3.valid == true)) {
        value = CamArray3.value;
        return true;
      }
      else if ((CamArray4.key == key) && (CamArray4.valid == true)) {
        value = CamArray4.value;
        return true;
      }
      else if ((CamArray5.key == key) && (CamArray5.valid == true)) {
        value = CamArray5.value;
        return true;
      }
      else if ((CamArray6.key == key) && (CamArray6.valid == true)) {
        value = CamArray6.value;
        return true;
      }
      else if ((CamArray7.key == key) && (CamArray7.valid == true)) {
        value = CamArray7.value;
        return true;
      }
      else {
        return false;
      }
    }

    /*******************************************************************************
     * @brief Reverse-search the CAM array for a key to a value.
     *
     * @param[in]  value   The value to lookup.
     * @param[out] key     The key corresponding to that value (or the first match).
     *
     * @return true if the the key was found.
     *******************************************************************************/
    bool reverse_lookup(V value, K &key)
    {
#pragma HLS pipeline II=1
#pragma HLS INLINE
      if ((CamArray0.value == value) && (CamArray0.valid == true)) {
        key = CamArray0.key;
        return true;
      }
      else if ((CamArray1.value == value) && (CamArray1.valid == true)) {
        key = CamArray1.key;
        return true;
      }
      else if ((CamArray2.value == value) && (CamArray2.valid == true)) {
        key = CamArray2.key;
        return true;
      }
      else if ((CamArray3.value == value) && (CamArray3.valid == true)) {
        key = CamArray3.key;
        return true;
      }
      else if ((CamArray4.value == value) && (CamArray4.valid == true)) {
        key = CamArray4.key;
        return true;
      }
      else if ((CamArray5.value == value) && (CamArray5.valid == true)) {
        key = CamArray5.key;
        return true;
      }
      else if ((CamArray6.value == value) && (CamArray6.valid == true)) {
        key = CamArray6.key;
        return true;
      }
      else if ((CamArray7.value == value) && (CamArray7.valid == true)) {
        key = CamArray7.key;
        return true;
      }
      else {
        return false;
      }
    }


    /*******************************************************************************
     * @brief Insert a new key-value pair in the CAM array.
     *
     * @param[in]  KeyValuePair  The key-value pair to insert.
     *
     * @return true if the the key was inserted.
     *******************************************************************************/
    bool insert(KeyValuePair<K,V> kVP)
    {
#pragma HLS pipeline II=1
#pragma HLS INLINE

      if (CamArray0.valid == false) {
        CamArray0 = kVP;
        return true;
      }
      else if (CamArray1.valid == false) {
        CamArray1 = kVP;
        return true;
      }
      else if (CamArray2.valid == false) {
        CamArray2 = kVP;
        return true;
      }
      else if (CamArray3.valid == false) {
        CamArray3 = kVP;
        return true;
      }
      else if (CamArray4.valid == false) {
        CamArray4 = kVP;
        return true;
      }
      else if (CamArray5.valid == false) {
        CamArray5 = kVP;
        return true;
      }
      else if (CamArray6.valid == false) {
        CamArray6 = kVP;
        return true;
      }
      else if (CamArray7.valid == false) {
        CamArray7 = kVP;
        return true;
      }
      else {
        return false;
      }
    }

    bool insert(K key, V value)
    {
#pragma HLS INLINE
      return insert(KeyValuePair<K,V>(key,value,true));
    }

    /*******************************************************************************
     * @brief Search the CAM array for a key and updates the corresponding value.
     *
     * @param[in]  key   The key to lookup.
     * @param[out] value The new value for that key
     *
     * @return true if the the key was found and updated
     *******************************************************************************/
    bool update(K key, V value)
    {
#pragma HLS pipeline II=1
#pragma HLS INLINE
      if ((CamArray0.key == key) && (CamArray0.valid == true)) {
        CamArray0.value = value;
        return true;
      }
      else if ((CamArray1.key == key) && (CamArray1.valid == true)) {
        CamArray1.value = value;
        return true;
      }
      else if ((CamArray2.key == key) && (CamArray2.valid == true)) {
        CamArray2.value = value;
        return true;
      }
      else if ((CamArray3.key == key) && (CamArray3.valid == true)) {
        CamArray3.value = value;
        return true;
      }
      else if ((CamArray4.key == key) && (CamArray4.valid == true)) {
        CamArray4.value = value;
        return true;
      }
      else if ((CamArray5.key == key) && (CamArray5.valid == true)) {
        CamArray5.value = value;
        return true;
      }
      else if ((CamArray6.key == key) && (CamArray6.valid == true)) {
        CamArray6.value = value;
        return true;
      }
      else if ((CamArray7.key == key) && (CamArray7.valid == true)) {
        CamArray7.value = value;
        return true;
      }
      else {
        return false;
      }
    }

    bool update(KeyValuePair<K,V> kVP)
    {
#pragma HLS INLINE
      return update(kVP.key, kVP.value);
    }

    /*******************************************************************************
     * @brief Remove a key-value pair from the CAM array.
     *
     * @param[in]  key  The key of the entry to be removed.
     *
     * @return true if the the key was deleted.
     ******************************************************************************/
    bool deleteEntry(K key)
    {
#pragma HLS pipeline II=1
#pragma HLS INLINE

      if ((CamArray0.key == key) && (CamArray0.valid == true)) {
        CamArray0.valid = false;
        return true;
      }
      else if ((CamArray1.key == key) && (CamArray1.valid == true)) {
        CamArray1.valid = false;
        return true;
      }
      else if ((CamArray2.key == key) && (CamArray2.valid == true)) {
        CamArray2.valid = false;
        return true;
      }
      else if ((CamArray3.key == key) && (CamArray3.valid == true)) {
        CamArray3.valid = false;
        return true;
      }
      else if ((CamArray4.key == key) && (CamArray4.valid == true)) {
        CamArray4.valid = false;
        return true;
      }
      else if ((CamArray5.key == key) && (CamArray5.valid == true)) {
        CamArray5.valid = false;
        return true;
      }
      else if ((CamArray6.key == key) && (CamArray6.valid == true)) {
        CamArray6.valid = false;
        return true;
      }
      else if ((CamArray7.key == key) && (CamArray7.valid == true)) {
        CamArray7.valid = false;
        return true;
      }
      else {
        return false;
      }
    }

    /*******************************************************************************
     * @brief Invalidate all entries of the CAM array.
     *
     ******************************************************************************/
    void reset()
    {
#pragma HLS pipeline II=1
#pragma HLS INLINE

      CamArray0.valid = false;
      CamArray1.valid = false;
      CamArray2.valid = false;
      CamArray3.valid = false;
      CamArray4.valid = false;
      CamArray5.valid = false;
      CamArray6.valid = false;
      CamArray7.valid = false;
    }
};


#endif

/*! \} */


