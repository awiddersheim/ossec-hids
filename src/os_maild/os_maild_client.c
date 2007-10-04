/* @(#) $Id$ */

/* Copyright (C) 2003-2006 Daniel B. Cid <dcid@ossec.net>
 * All rights reserved.
 *
 * This program is a free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public
 * License (version 3) as published by the FSF - Free Software
 * Foundation
 */


#include "shared.h"
#include "maild.h"


/* OS_RecvMailQ, 
 * v0.1, 2005/03/15
 * Receive a Message on the Mail queue
 * v0,2: Using the new file-queue.
 */
MailMsg *OS_RecvMailQ(file_queue *fileq, struct tm *p, 
                      MailConfig *Mail, MailMsg **msg_sms)
{
    int i = 0, body_size = OS_MAXSTR -3, log_size, sms_set = 0,donotgroup = 0;
    char logs[OS_MAXSTR + 1];
    char *subject_host;
    
    MailMsg *mail;
    alert_data *al_data;

    Mail->priority = 0;


    /* Get message if available */
    al_data = Read_FileMon(fileq, p, mail_timeout);
    if(!al_data)
        return(NULL);


    /* If e-mail came correctly, generate the e-mail body/subject */
    os_calloc(1,sizeof(MailMsg), mail);
    os_calloc(BODY_SIZE, sizeof(char), mail->body);
    os_calloc(SUBJECT_SIZE, sizeof(char), mail->subject);


    /* Generating the logs */
    logs[0] = '\0';
    logs[OS_MAXSTR] = '\0';
    
    while(al_data->log[i])
    {
        log_size = strlen(al_data->log[i]) + 4;
        
        /* If size left is small than the size of the log, stop it */
        if(body_size <= log_size)
        {
            break;
        }
        
        strncat(logs, al_data->log[i], body_size);
        strncat(logs, "\r\n", body_size);
        body_size -= log_size;
        i++;
    }


    /* Subject */
    subject_host = strchr(al_data->location, '>');
    if(subject_host)
    {
        subject_host--;
        *subject_host = '\0';
    }

    /* We have two subject options - full and normal */
    if(Mail->subject_full)
    {
        /* Option for a clean full subject (without ossec in the name) */
        #ifdef CLEANFULL
        snprintf(mail->subject, SUBJECT_SIZE -1, MAIL_SUBJECT_FULL2, 
                                al_data->level,
                                al_data->comment,
                                al_data->location);
        #else
        snprintf(mail->subject, SUBJECT_SIZE -1, MAIL_SUBJECT_FULL, 
                                al_data->location,
                                al_data->level,
                                al_data->comment);
        #endif
    }
    else
    {
        snprintf(mail->subject, SUBJECT_SIZE -1, MAIL_SUBJECT, 
                                             al_data->location,
                                             al_data->level);
    }

    
    /* fixing subject back */
    if(subject_host)
    {
        *subject_host = '-';
    }

    
    /* Body */
    snprintf(mail->body, BODY_SIZE -1, MAIL_BODY,
            al_data->date,
            al_data->location,
            al_data->rule,
            al_data->level,
            al_data->comment,
            logs);


    /* Checking for granular email configs */
    if(Mail->gran_to)
    {
        i = 0;
        while(Mail->gran_to[i] != NULL)
        {
            int gr_set = 0;
            
            /* Looking if location is set */
            if(Mail->gran_location[i])
            {
                if(OSMatch_Execute(al_data->location,
                                   strlen(al_data->location),
                                   Mail->gran_location[i]))
                {
                    gr_set = 1;
                }
                else
                {
                    i++;
                    continue;
                }
            }
            
            /* Looking for the level */
            if(Mail->gran_level[i])
            {
                if(al_data->level >= Mail->gran_level[i])
                {
                    gr_set = 1;
                }
                else
                {
                    i++;
                    continue;
                }
            }


            /* Looking for rule id */
            if(Mail->gran_id[i])
            {
                int id_i = 0;
                while(Mail->gran_id[i][id_i] != 0)
                {
                    if(Mail->gran_id[i][id_i] == al_data->rule)
                    {
                        break;
                    }
                    id_i++;
                }

                /* If we found, id is going to be a valid rule */
                if(Mail->gran_id[i][id_i])
                {
                    gr_set = 1;
                }
                else
                {
                    i++;
                    continue;
                }
            }
            

            /* Looking for the group */
            if(Mail->gran_group[i])
            {
                if(OSMatch_Execute(al_data->group,
                                   strlen(al_data->group),
                                   Mail->gran_group[i]))
                {
                    gr_set = 1;
                }
                else
                {
                    i++;
                    continue;
                }
            }


            /* If we got in here, it is because everything
             * matched. Set this e-mail to be used.
             */
            if(gr_set)
            {
                if(Mail->gran_format[i] == SMS_FORMAT)
                {
                    Mail->gran_set[i] = SMS_FORMAT;

                    /* Setting the SMS flag */
                    sms_set = 1;
                }
                else
                {
                    /* Options */
                    if(Mail->gran_format[i] == FORWARD_NOW)
                    {
                        Mail->priority = 1;
                        Mail->gran_set[i] = FULL_FORMAT;
                    }
                    else if(Mail->gran_format[i] == DONOTGROUP)
                    {
                        Mail->priority = DONOTGROUP;
                        Mail->gran_set[i] = DONOTGROUP;
                        donotgroup = 1;
                    }
                    else
                    {
                        Mail->gran_set[i] = FULL_FORMAT;
                    }
                }
            }
            i++;
        }
    }


    /* If DONOTGROUP is set, we can't assign the new subject */
    if(!donotgroup)
    {
        /* Getting highest level for alert */
        if(_g_subject[0] != '\0')
        {
            if(_g_subject_level < al_data->level)
            {
                strncpy(_g_subject, mail->subject, SUBJECT_SIZE);
                _g_subject_level = al_data->level;
            }
        }
        else
        {
            strncpy(_g_subject, mail->subject, SUBJECT_SIZE);
            _g_subject_level = al_data->level;
        }
    }
    
    
    /* If sms is set, create the sms output */
    if(sms_set)
    {
        MailMsg *msg_sms_tmp;
        
        /* Allocate memory for sms */
        os_calloc(1,sizeof(MailMsg), msg_sms_tmp);
        os_calloc(BODY_SIZE, sizeof(char), msg_sms_tmp->body);
        os_calloc(SUBJECT_SIZE, sizeof(char), msg_sms_tmp->subject);

        snprintf(msg_sms_tmp->subject, SUBJECT_SIZE -1, SMS_SUBJECT,
                                      al_data->level,
                                      al_data->rule,
                                      al_data->comment);


        strncpy(msg_sms_tmp->body, logs, 128);
        msg_sms_tmp->body[127] = '\0';
        
        /* Assigning msg_sms */
        *msg_sms = msg_sms_tmp;
    }
    
    
    
    /* Clearing the memory */
    FreeAlertData(al_data);

    
    return(mail);

}
/* EOF */
