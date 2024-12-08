/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.1  (the "License"); you may not use this file
 * except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.openairinterface.org/?page_id=698
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------------------
 * For more information about the OpenAirInterface (OAI) Software Alliance:
 *      contact@openairinterface.org
 */

#include "../../../../src/xApp/e42_xapp_api.h"
#include "../../../../src/sm/rc_sm/ie/ir/ran_param_struct.h"
#include "../../../../src/sm/rc_sm/ie/ir/ran_param_list.h"
#include "../../../../src/util/time_now_us.h"
#include "../../../../src/util/alg_ds/ds/lock_guard/lock_guard.h"
#include "../../../../src/sm/rc_sm/rc_sm_id.h"
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/socket.h>


#define PORT 8080
#define MAX_CLIENTS 2
#define BUFFER_SIZE 2000
#define SERVER_IP "192.168.70.1" // Replace with your server's IP address

typedef struct {
    int socket;
    int client_id;
} client_data_t;

typedef struct {
    int sst;
    int sd;
    int normal_count;
    int anomaly_count;
    int prb_allocation;
    int prev_prb_allocation;
    int rrc_ue_id;
} ue_data_t;


ue_data_t ue_data[MAX_CLIENTS];
int client_sockets[MAX_CLIENTS];
int clients_connected = 0;
int messages_received = 0;

pthread_mutex_t lock;
pthread_cond_t cond;

typedef enum{
    DRX_parameter_configuration_7_6_3_1 = 1,
    SR_periodicity_configuration_7_6_3_1 = 2,
    SPS_parameters_configuration_7_6_3_1 = 3,
    Configured_grant_control_7_6_3_1 = 4,
    CQI_table_configuration_7_6_3_1 = 5,
    Slice_level_PRB_quotal_7_6_3_1 = 6,
} rc_ctrl_service_style_2_act_id_e;

static
e2sm_rc_ctrl_hdr_frmt_1_t gen_rc_ctrl_hdr_frmt_1(ue_id_e2sm_t ue_id, uint32_t ric_style_type, uint16_t ctrl_act_id)
{
  e2sm_rc_ctrl_hdr_frmt_1_t dst = {0};

  // 6.2.2.6
  dst.ue_id = cp_ue_id_e2sm(&ue_id);

  dst.ric_style_type = ric_style_type;
  dst.ctrl_act_id = ctrl_act_id;

  return dst;
}

static
e2sm_rc_ctrl_hdr_t gen_rc_ctrl_hdr(e2sm_rc_ctrl_hdr_e hdr_frmt, ue_id_e2sm_t ue_id, uint32_t ric_style_type, uint16_t ctrl_act_id)
{
  e2sm_rc_ctrl_hdr_t dst = {0};

  if (hdr_frmt == FORMAT_1_E2SM_RC_CTRL_HDR) {
    dst.format = FORMAT_1_E2SM_RC_CTRL_HDR;
    dst.frmt_1 = gen_rc_ctrl_hdr_frmt_1(ue_id, ric_style_type, ctrl_act_id);
  } else {
    assert(0!=0 && "not implemented the fill func for this ctrl hdr frmt");
  }

  return dst;
}

typedef enum {
    RRM_Policy_Ratio_List_8_4_3_6 = 1,
    RRM_Policy_Ratio_Group_8_4_3_6 = 2,
    RRM_Policy_8_4_3_6 = 3,
    RRM_Policy_Member_List_8_4_3_6 = 4,
    RRM_Policy_Member_8_4_3_6 = 5,
    PLMN_Identity_8_4_3_6 = 6,
    S_NSSAI_8_4_3_6 = 7,
    SST_8_4_3_6 = 8,
    SD_8_4_3_6 = 9,
    Min_PRB_Policy_Ratio_8_4_3_6 = 10,
    Max_PRB_Policy_Ratio_8_4_3_6 = 11,
    Dedicated_PRB_Policy_Ratio_8_4_3_6 = 12,
} slice_level_PRB_quota_param_id_e;

static
void gen_rrm_policy_ratio_group(lst_ran_param_t* RRM_Policy_Ratio_Group,
                                const char* sst_str,
                                const char* sd_str,
                                int min_ratio_prb,
                                int dedicated_ratio_prb,
                                int max_ratio_prb)
{
  // RRM Policy Ratio Group, STRUCTURE (RRM Policy Ratio List -> RRM Policy Ratio Group)
  // lst_ran_param_t* RRM_Policy_Ratio_Group = &RRM_Policy_Ratio_List->ran_param_val.lst->lst_ran_param[0];
  // RRM_Policy_Ratio_Group->ran_param_id = RRM_Policy_Ratio_Group_8_4_3_6;
  RRM_Policy_Ratio_Group->ran_param_struct.sz_ran_param_struct = 4;
  RRM_Policy_Ratio_Group->ran_param_struct.ran_param_struct = calloc(4, sizeof(seq_ran_param_t));
  assert(RRM_Policy_Ratio_Group->ran_param_struct.ran_param_struct != NULL && "Memory exhausted");
  // RRM Policy, STRUCTURE (RRM Policy Ratio Group -> RRM Policy)
  seq_ran_param_t* RRM_Policy = &RRM_Policy_Ratio_Group->ran_param_struct.ran_param_struct[0];
  RRM_Policy->ran_param_id = RRM_Policy_8_4_3_6;
  RRM_Policy->ran_param_val.type = STRUCTURE_RAN_PARAMETER_VAL_TYPE;
  RRM_Policy->ran_param_val.strct = calloc(1, sizeof(ran_param_struct_t));
  assert(RRM_Policy->ran_param_val.strct != NULL && "Memory exhausted");
  RRM_Policy->ran_param_val.strct->sz_ran_param_struct = 1;
  RRM_Policy->ran_param_val.strct->ran_param_struct = calloc(1, sizeof(seq_ran_param_t));
  assert(RRM_Policy->ran_param_val.strct->ran_param_struct != NULL && "Memory exhausted");
  // RRM Policy Member List, LIST (RRM Policy -> RRM Policy Member List)
  seq_ran_param_t* RRM_Policy_Member_List = &RRM_Policy->ran_param_val.strct->ran_param_struct[0];
  RRM_Policy_Member_List->ran_param_id = RRM_Policy_Member_List_8_4_3_6;
  RRM_Policy_Member_List->ran_param_val.type = LIST_RAN_PARAMETER_VAL_TYPE;
  RRM_Policy_Member_List->ran_param_val.lst = calloc(1, sizeof(ran_param_list_t));
  assert(RRM_Policy_Member_List->ran_param_val.lst != NULL && "Memory exhausted");
  RRM_Policy_Member_List->ran_param_val.lst->sz_lst_ran_param = 1;
  RRM_Policy_Member_List->ran_param_val.lst->lst_ran_param = calloc(1, sizeof(lst_ran_param_t));
  assert(RRM_Policy_Member_List->ran_param_val.lst->lst_ran_param != NULL && "Memory exhausted");
  // RRM Policy Member, STRUCTURE (RRM Policy Member List -> RRM Policy Member)
  lst_ran_param_t* RRM_Policy_Member = &RRM_Policy_Member_List->ran_param_val.lst->lst_ran_param[0];
  // RRM_Policy_Member->ran_param_id = RRM_Policy_Member_8_4_3_6;
  RRM_Policy_Member->ran_param_struct.sz_ran_param_struct = 2;
  RRM_Policy_Member->ran_param_struct.ran_param_struct = calloc(2, sizeof(seq_ran_param_t));
  assert(RRM_Policy_Member->ran_param_struct.ran_param_struct != NULL && "Memory exhausted");
  // PLMN Identity, ELEMENT (RRM Policy Member -> PLMN Identity)
  seq_ran_param_t* PLMN_Identity = &RRM_Policy_Member->ran_param_struct.ran_param_struct[0];
  PLMN_Identity->ran_param_id = PLMN_Identity_8_4_3_6;
  PLMN_Identity->ran_param_val.type = ELEMENT_KEY_FLAG_FALSE_RAN_PARAMETER_VAL_TYPE;
  PLMN_Identity->ran_param_val.flag_false = calloc(1, sizeof(ran_parameter_value_t));
  assert(PLMN_Identity->ran_param_val.flag_false != NULL && "Memory exhausted");
  PLMN_Identity->ran_param_val.flag_false->type = OCTET_STRING_RAN_PARAMETER_VALUE;
  char plmnid_str[] = "00101";
  byte_array_t plmn_id = cp_str_to_ba(plmnid_str); // TODO
  PLMN_Identity->ran_param_val.flag_false->octet_str_ran.len = plmn_id.len;
  PLMN_Identity->ran_param_val.flag_false->octet_str_ran.buf = plmn_id.buf;
  // S-NSSAI, STRUCTURE (RRM Policy Member -> S-NSSAI)
  seq_ran_param_t* S_NSSAI = &RRM_Policy_Member->ran_param_struct.ran_param_struct[1];
  S_NSSAI->ran_param_id = S_NSSAI_8_4_3_6;
  S_NSSAI->ran_param_val.type = STRUCTURE_RAN_PARAMETER_VAL_TYPE;
  S_NSSAI->ran_param_val.strct = calloc(1, sizeof(ran_param_struct_t));
  assert(S_NSSAI->ran_param_val.strct != NULL && "Memory exhausted");
  S_NSSAI->ran_param_val.strct->sz_ran_param_struct = 2;
  S_NSSAI->ran_param_val.strct->ran_param_struct = calloc(2, sizeof(seq_ran_param_t));
  // SST, ELEMENT (S-NSSAI -> SST)
  seq_ran_param_t* SST = &S_NSSAI->ran_param_val.strct->ran_param_struct[0];
  SST->ran_param_id = SST_8_4_3_6;
  SST->ran_param_val.type = ELEMENT_KEY_FLAG_FALSE_RAN_PARAMETER_VAL_TYPE;
  SST->ran_param_val.flag_false = calloc(1, sizeof(ran_parameter_value_t));
  assert(SST->ran_param_val.flag_false != NULL && "Memory exhausted");
  SST->ran_param_val.flag_false->type = OCTET_STRING_RAN_PARAMETER_VALUE;
  // char sst_str[] = "1";
  byte_array_t sst = cp_str_to_ba(sst_str); //TODO
  SST->ran_param_val.flag_false->octet_str_ran.len = sst.len;
  SST->ran_param_val.flag_false->octet_str_ran.buf = sst.buf;
  // SD, ELEMENT (S-NSSAI -> SD)
  seq_ran_param_t* SD = &S_NSSAI->ran_param_val.strct->ran_param_struct[1];
  SD->ran_param_id = SD_8_4_3_6;
  SD->ran_param_val.type = ELEMENT_KEY_FLAG_FALSE_RAN_PARAMETER_VAL_TYPE;
  SD->ran_param_val.flag_false = calloc(1, sizeof(ran_parameter_value_t));
  assert(SD->ran_param_val.flag_false != NULL && "Memory exhausted");
  SD->ran_param_val.flag_false->type = OCTET_STRING_RAN_PARAMETER_VALUE;
  // char sd_str[] = "0";
  byte_array_t sd = cp_str_to_ba(sd_str); //TODO
  SD->ran_param_val.flag_false->octet_str_ran.len = sd.len;
  SD->ran_param_val.flag_false->octet_str_ran.buf = sd.buf;
  // Min PRB Policy Ratio, ELEMENT (RRM Policy Ratio Group -> Min PRB Policy Ratio)
  seq_ran_param_t* Min_PRB_Policy_Ratio = &RRM_Policy_Ratio_Group->ran_param_struct.ran_param_struct[1];
  Min_PRB_Policy_Ratio->ran_param_id = Min_PRB_Policy_Ratio_8_4_3_6;
  Min_PRB_Policy_Ratio->ran_param_val.type = ELEMENT_KEY_FLAG_FALSE_RAN_PARAMETER_VAL_TYPE;
  Min_PRB_Policy_Ratio->ran_param_val.flag_false = calloc(1, sizeof(ran_parameter_value_t));
  assert(Min_PRB_Policy_Ratio->ran_param_val.flag_false != NULL && "Memory exhausted");
  Min_PRB_Policy_Ratio->ran_param_val.flag_false->type = INTEGER_RAN_PARAMETER_VALUE;
  // TODO: not handle this value in OAI
  Min_PRB_Policy_Ratio->ran_param_val.flag_false->int_ran = min_ratio_prb;
  // Max PRB Policy Ratio, ELEMENT (RRM Policy Ratio Group -> Max PRB Policy Ratio)
  seq_ran_param_t* Max_PRB_Policy_Ratio = &RRM_Policy_Ratio_Group->ran_param_struct.ran_param_struct[2];
  Max_PRB_Policy_Ratio->ran_param_id = Max_PRB_Policy_Ratio_8_4_3_6;
  Max_PRB_Policy_Ratio->ran_param_val.type = ELEMENT_KEY_FLAG_FALSE_RAN_PARAMETER_VAL_TYPE;
  Max_PRB_Policy_Ratio->ran_param_val.flag_false = calloc(1, sizeof(ran_parameter_value_t));
  assert(Max_PRB_Policy_Ratio->ran_param_val.flag_false != NULL && "Memory exhausted");
  Max_PRB_Policy_Ratio->ran_param_val.flag_false->type = INTEGER_RAN_PARAMETER_VALUE;
  // TODO: not handle this value in OAI
  Max_PRB_Policy_Ratio->ran_param_val.flag_false->int_ran = max_ratio_prb;
  // Dedicated PRB Policy Ratio, ELEMENT (RRM Policy Ratio Group -> Dedicated PRB Policy Ratio)
  seq_ran_param_t* Dedicated_PRB_Policy_Ratio = &RRM_Policy_Ratio_Group->ran_param_struct.ran_param_struct[3];
  Dedicated_PRB_Policy_Ratio->ran_param_id = Dedicated_PRB_Policy_Ratio_8_4_3_6;
  Dedicated_PRB_Policy_Ratio->ran_param_val.type = ELEMENT_KEY_FLAG_FALSE_RAN_PARAMETER_VAL_TYPE;
  Dedicated_PRB_Policy_Ratio->ran_param_val.flag_false = calloc(1, sizeof(ran_parameter_value_t));
  assert(Dedicated_PRB_Policy_Ratio->ran_param_val.flag_false != NULL && "Memory exhausted");
  Dedicated_PRB_Policy_Ratio->ran_param_val.flag_false->type = INTEGER_RAN_PARAMETER_VALUE;
  Dedicated_PRB_Policy_Ratio->ran_param_val.flag_false->int_ran = dedicated_ratio_prb;

  return;
}

static void gen_rrm_policy_ratio_list(seq_ran_param_t* RRM_Policy_Ratio_List) {
    int num_slice = MAX_CLIENTS;
    RRM_Policy_Ratio_List->ran_param_id = RRM_Policy_Ratio_List_8_4_3_6;
    RRM_Policy_Ratio_List->ran_param_val.type = LIST_RAN_PARAMETER_VAL_TYPE;
    RRM_Policy_Ratio_List->ran_param_val.lst = calloc(1, sizeof(ran_param_list_t));
    assert(RRM_Policy_Ratio_List->ran_param_val.lst != NULL && "Memory exhausted");
    RRM_Policy_Ratio_List->ran_param_val.lst->sz_lst_ran_param = num_slice;
    RRM_Policy_Ratio_List->ran_param_val.lst->lst_ran_param = calloc(num_slice, sizeof(lst_ran_param_t));
    assert(RRM_Policy_Ratio_List->ran_param_val.lst->lst_ran_param != NULL && "Memory exhausted");

    for (int i = 0; i < num_slice; i++) {
        char sst_str[2];
        char sd_str[2];
        snprintf(sst_str, sizeof(sst_str), "%d", ue_data[i].sst);
        snprintf(sd_str, sizeof(sd_str), "%d", ue_data[i].sd);
        gen_rrm_policy_ratio_group(&RRM_Policy_Ratio_List->ran_param_val.lst->lst_ran_param[i],
                                   sst_str,
                                   sd_str,
                                   0, ue_data[i].prb_allocation, 0);
    }

    return;
}



static e2sm_rc_ctrl_msg_frmt_1_t gen_rc_ctrl_msg_frmt_1_slice_level_PRB_quota() {
    e2sm_rc_ctrl_msg_frmt_1_t dst = {0};

  // 8.4.3.6
  // RRM Policy Ratio List, LIST (len 1)
  // > RRM Policy Ratio Group, STRUCTURE (len 4)
  // >>  RRM Policy, STRUCTURE (len 1)
  // >>> RRM Policy Member List, LIST (len 1)
  // >>>> RRM Policy Member, STRUCTURE (len 2)
  // >>>>> PLMN Identity, ELEMENT
  // >>>>> S-NSSAI, STRUCTURE (len 2)
  // >>>>>> SST, ELEMENT
  // >>>>>> SD, ELEMENT
  // >> Min PRB Policy Ratio, ELEMENT
  // >> Max PRB Policy Ratio, ELEMENT
  // >> Dedicated PRB Policy Ratio, ELEMENT

    // RRM Policy Ratio List, LIST
    dst.sz_ran_param = 1;
    dst.ran_param = calloc(1, sizeof(seq_ran_param_t));
    assert(dst.ran_param != NULL && "Memory exhausted");
    gen_rrm_policy_ratio_list(&dst.ran_param[0]);

    return dst;
}

static e2sm_rc_ctrl_msg_t gen_rc_ctrl_msg(e2sm_rc_ctrl_msg_e msg_frmt) {
    e2sm_rc_ctrl_msg_t dst = {0};

    if (msg_frmt == FORMAT_1_E2SM_RC_CTRL_MSG) {
        dst.format = msg_frmt;
        dst.frmt_1 = gen_rc_ctrl_msg_frmt_1_slice_level_PRB_quota();
    } else {
        assert(0!=0 && "not implemented the fill func for this ctrl msg frmt");
    }

    return dst;
}



static
ue_id_e2sm_t gen_rc_ue_id(ue_id_e2sm_e type)
{
  ue_id_e2sm_t ue_id = {0};
  if (type == GNB_UE_ID_E2SM) {
    ue_id.type = GNB_UE_ID_E2SM;
    // TODO
    ue_id.gnb.amf_ue_ngap_id = 0;
    ue_id.gnb.guami.plmn_id.mcc = 1;
    ue_id.gnb.guami.plmn_id.mnc = 1;
    ue_id.gnb.guami.plmn_id.mnc_digit_len = 2;
    ue_id.gnb.guami.amf_region_id = 0;
    ue_id.gnb.guami.amf_set_id = 0;
    ue_id.gnb.guami.amf_ptr = 0;
  } else {
    assert(0!=0 && "not supported UE ID type");
  }
  return ue_id;
}


void *client_handler(void *client_data) {
    client_data_t *data = (client_data_t*)client_data;
    int sock = data->socket;
    int client_id = data->client_id;
    int read_size;
    char client_message[BUFFER_SIZE];

    while ((read_size = recv(sock, client_message, BUFFER_SIZE, 0)) > 0) {
        client_message[read_size] = '\0';

        // Print the received message for debugging
        printf("Received message from client %d: %s\n", client_id + 1, client_message);

        int sst, sd, normal_count, anomaly_count;
        if (sscanf(client_message, "sst:%d,sd:%d,normal:%d,anomaly:%d", &sst, &sd, &normal_count, &anomaly_count) == 4) {
            pthread_mutex_lock(&lock);
            ue_data[client_id].sst = sst;
            ue_data[client_id].sd = sd;
            ue_data[client_id].normal_count = normal_count;
            ue_data[client_id].anomaly_count = anomaly_count;
            if (sd ==  1) {
                ue_data[client_id].rrc_ue_id = 1;
            }
            else{
                ue_data[client_id].rrc_ue_id = 2;
            }
            messages_received++;
            if (messages_received == MAX_CLIENTS) {
                pthread_cond_signal(&cond);
            }
            pthread_mutex_unlock(&lock);

            // Print the parsed values for debugging
            printf("Parsed values for client %d: SST: %d, SD: %d, Normal: %d, Anomaly: %d\n",
                   client_id + 1, sst, sd, normal_count, anomaly_count);
        } else {
            printf("Failed to parse message from client %d: %s\n", client_id + 1, client_message);
        }
    }

    if (read_size == 0) {
        printf("Client %d disconnected\n", client_id + 1);
        fflush(stdout);
    } else if (read_size == -1) {
        perror("recv failed");
    }

    free(client_data);
    return 0;
}

// Function to execute RRC release command
void* rrc_release_ue_thread(void* arg) {
    int ran_ue_id = *((int*)arg);
    char command[256];
    snprintf(command, sizeof(command), "echo rrc release_rrc %d | nc %s 9090", ran_ue_id, SERVER_IP);
    system(command);
    free(arg);
    return NULL;
}

void rrc_release_ue(int ran_ue_id) {
    pthread_t thread;
    int* arg = malloc(sizeof(*arg));
    if (arg) {
        *arg = ran_ue_id;
        pthread_create(&thread, NULL, rrc_release_ue_thread, arg);
        pthread_detach(thread);  // Detach the thread to avoid memory leaks
    }
}

void enforce_slicing(e2_node_arr_xapp_t nodes) {
    // Enforce slicing
    rc_ctrl_req_data_t rc_ctrl = {0};
    ue_id_e2sm_t ue_id = gen_rc_ue_id(GNB_UE_ID_E2SM);
    rc_ctrl.hdr = gen_rc_ctrl_hdr(FORMAT_1_E2SM_RC_CTRL_HDR, ue_id, 2, Slice_level_PRB_quotal_7_6_3_1);
    rc_ctrl.msg = gen_rc_ctrl_msg(FORMAT_1_E2SM_RC_CTRL_MSG);

    for (size_t i = 0; i < nodes.len; ++i) {
        control_sm_xapp_api(&nodes.n[i].id, SM_RC_ID, &rc_ctrl);
    }
    free_rc_ctrl_req_data(&rc_ctrl);
}

void parse_and_apply_slicing(e2_node_arr_xapp_t nodes) {
    int total_packets[MAX_CLIENTS];
    double anomaly_ratios[MAX_CLIENTS];
    int total_prb_allocation = 0;
    int prb_allocation[MAX_CLIENTS];
    int attacker_index = -1;

    // Calculate anomaly ratios and tentative PRB allocations
    for (int i = 0; i < MAX_CLIENTS; i++) {
        total_packets[i] = ue_data[i].normal_count + ue_data[i].anomaly_count;
        anomaly_ratios[i] = (total_packets[i] > 0) ? (double)ue_data[i].anomaly_count / total_packets[i] : 0;
        prb_allocation[i] = (int)((1.0 - anomaly_ratios[i]) * 100);
        total_prb_allocation += prb_allocation[i];
        if (anomaly_ratios[i] == 1.0) {
            // Anomaly ratio is 100%, mark as attacker
            attacker_index = i;
        }
    }

    if (attacker_index != -1) {
        // Set PRB allocation to 0% for the attacker
        prb_allocation[attacker_index] = 0;
        ue_data[attacker_index].prb_allocation = 0;

        // Distribute remaining PRB among other UEs
        total_prb_allocation = 0;
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (i != attacker_index) {
                prb_allocation[i] = (int)((1.0 - anomaly_ratios[i]) * 100);
                total_prb_allocation += prb_allocation[i];
            }
        }

        if (total_prb_allocation > 100) {
            double scaling_factor = 100.0 / total_prb_allocation;
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (i != attacker_index) {
                    prb_allocation[i] = (int)(prb_allocation[i] * scaling_factor);
                }
            }
        }

        // Apply new PRB allocations before RRC release
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (i != attacker_index) {
                if (prb_allocation[i] < ue_data[i].prev_prb_allocation) {
                    ue_data[i].prb_allocation = prb_allocation[i];
                    enforce_slicing(nodes);
                    ue_data[i].prev_prb_allocation = ue_data[i].prb_allocation;
                }
            }
        }

        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (i != attacker_index) {
                if (prb_allocation[i] > ue_data[i].prev_prb_allocation) {
                    ue_data[i].prb_allocation = prb_allocation[i];
                    enforce_slicing(nodes);
                    ue_data[i].prev_prb_allocation = ue_data[i].prb_allocation;
                }
            }
        }

        // Trigger RRC release for the attacker
        printf("Client %d (RAN UE ID: %d) has 100%% anomaly. Triggering RRC release.\n", attacker_index + 1, ue_data[attacker_index].rrc_ue_id);
        rrc_release_ue(ue_data[attacker_index].rrc_ue_id);
    } else {
        // Adjust PRB allocations to ensure the total does not exceed 100%
        if (total_prb_allocation > 100) {
            double scaling_factor = 100.0 / total_prb_allocation;
            for (int i = 0; i < MAX_CLIENTS; i++) {
                prb_allocation[i] = (int)(prb_allocation[i] * scaling_factor);
            }
        }

        // Apply slicing changes
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (prb_allocation[i] < ue_data[i].prev_prb_allocation) {
                ue_data[i].prb_allocation = prb_allocation[i];
                enforce_slicing(nodes);
                ue_data[i].prev_prb_allocation = ue_data[i].prb_allocation;
            }
        }

        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (prb_allocation[i] > ue_data[i].prev_prb_allocation) {
                ue_data[i].prb_allocation = prb_allocation[i];
                enforce_slicing(nodes);
                ue_data[i].prev_prb_allocation = ue_data[i].prb_allocation;
            }
        }
    }

    // Print the final allocations
    for (int i = 0; i < MAX_CLIENTS; i++) {
        printf("Client %d (SST: %d, SD: %d) PRB allocation: %d%%\n", i + 1, ue_data[i].sst, ue_data[i].sd, ue_data[i].prb_allocation);
    }
}



int main(int argc, char *argv[]) {
    int socket_desc, new_socket, c;
    struct sockaddr_in server, client;
    int opt = 1;

    // Initialize mutex and condition variable
    pthread_mutex_init(&lock, NULL);
    pthread_cond_init(&cond, NULL);

    // Initialize ue_data with default SST and SD values
    for (int i = 0; i < MAX_CLIENTS; i++) {
        ue_data[i].sst = (i == 0) ? 1 : 1;
        ue_data[i].sd = (i == 0) ? 1 : 5;
        ue_data[i].normal_count = 0;
        ue_data[i].anomaly_count = 0;
        ue_data[i].prb_allocation = 0;
        ue_data[i].prev_prb_allocation = 0;
    }

    fr_args_t args = init_fr_args(argc, argv);
    //defer({ free_fr_args(&args); });

    // Init the xApp
    init_xapp_api(&args);
    sleep(1);

    e2_node_arr_xapp_t nodes = e2_nodes_xapp_api();
    defer({ free_e2_node_arr_xapp(&nodes); });
    assert(nodes.len > 0);
    printf("Connected E2 nodes = %d\n", nodes.len);

    // Create socket
    socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_desc == -1) {
        printf("Could not create socket");
        return 1;
    }
    puts("Socket created");

    // Set the socket options
    if (setsockopt(socket_desc, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt failed");
        return 1;
    }

    // Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(SERVER_IP); // Use defined IP address
    server.sin_port = htons(PORT);

    // Bind
    if (bind(socket_desc, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("Bind failed. Error");
        return 1;
    }
    puts("Bind done");

    // Listen
    listen(socket_desc, MAX_CLIENTS);

    // Accept incoming connections
    puts("Waiting for incoming connections...");
    c = sizeof(struct sockaddr_in);

    for (int i = 0; i < MAX_CLIENTS; i++) {
        new_socket = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c);
        if (new_socket < 0) {
            perror("Accept failed");
            return 1;
        }
        printf("Connection accepted from client %d\n", i + 1);

        client_data_t *client_data = (client_data_t*)malloc(sizeof(client_data_t));
        client_data->socket = new_socket;
        client_data->client_id = i;

        pthread_mutex_lock(&lock);
        clients_connected++;
        pthread_cond_signal(&cond);
        pthread_mutex_unlock(&lock);

        pthread_t sniffer_thread;
        if (pthread_create(&sniffer_thread, NULL, client_handler, (void*)client_data) < 0) {
            perror("could not create thread");
            return 1;
        }

        pthread_detach(sniffer_thread);
    }

    // Wait until both clients are connected
    pthread_mutex_lock(&lock);
    while (clients_connected < MAX_CLIENTS) {
        pthread_cond_wait(&cond, &lock);
    }
    pthread_mutex_unlock(&lock);

    puts("Both clients connected. Starting main loop...");

    ////////////
    // START RC
    ////////////

    // RC Control
    // CONTROL Service Style 2: Radio Resource Allocation Control
    // Action ID 6: Slice-level PRB quota
    // E2SM-RC Control Header Format 1
    // E2SM-RC Control Message Format 1
    rc_ctrl_req_data_t rc_ctrl = {0};
    ue_id_e2sm_t ue_id = gen_rc_ue_id(GNB_UE_ID_E2SM);

        // Initial dummy allocation
    for (int i = 0; i < MAX_CLIENTS; i++) {
        ue_data[i].sst = 1;
        ue_data[i].sd = (i == 0) ? 1 : 5;
        ue_data[i].prb_allocation = 50; // Default to 50% allocation
        ue_data[i].prev_prb_allocation = 50;
    }

    rc_ctrl.hdr = gen_rc_ctrl_hdr(FORMAT_1_E2SM_RC_CTRL_HDR, ue_id, 2, Slice_level_PRB_quotal_7_6_3_1);
    rc_ctrl.msg = gen_rc_ctrl_msg(FORMAT_1_E2SM_RC_CTRL_MSG);

    
    for (size_t i = 0; i < nodes.len; ++i) {
              control_sm_xapp_api(&nodes.n[i].id, SM_RC_ID, &rc_ctrl);
    }
    free_rc_ctrl_req_data(&rc_ctrl);
    puts("RC initialization completed. Starting main loop...");

    // Main loop to process messages
    while (1) {
        pthread_mutex_lock(&lock);
        while (messages_received < MAX_CLIENTS) {
            pthread_cond_wait(&cond, &lock);
        }
        // Print received messages
        for (int i = 0; i < MAX_CLIENTS; i++) {
            printf("Client %d: SST: %d, SD: %d, Normal: %d, Anomaly: %d\n",
                   i + 1, ue_data[i].sst, ue_data[i].sd, ue_data[i].normal_count, ue_data[i].anomaly_count);
        }

        // Parse and apply slicing based on received messages
        parse_and_apply_slicing(nodes);

        messages_received = 0;
        pthread_mutex_unlock(&lock);
    }

    // Cleanup and close sockets
    close(socket_desc);
    pthread_mutex_destroy(&lock);
    pthread_cond_destroy(&cond);

  ////////////
  // END RC
  ////////////

  //Stop the xApp
  while(try_stop_xapp_api() == false)
    usleep(1000);

  return 0;
}