# Moved to Github

This repository was moved to Github. My original repository has >4 years of activity from 2020-09-28 to 2024-11-18: https://bitbucket.org/marcosjom/thinstream-src/

# sys-thinstream-src

This is a Real-Time-Stream-Protocol (RTSP) based cameras recorder and distribution service.

Created by [Marcos Ortega](https://mortegam.com/), in use serving https://mortegam.com/

Built on top of [sys-nbframework-src](https://github.com/marcosjom/sys-nbframework-src), this project serves as demostration of how to implement your app's API webserver when you build an app or service with [sys-nbframework-src](https://github.com/marcosjom/sys-nbframework-src).

# Features

- RTSP/RTC/RTCP supported.
- H.264 NALUs parsing.
- io-events driven implementation, one or few threads can handle many requests with concurrency.
- HTTP/HTTPS server integrated.
- SSL (HTTPS) support.
- compiled for Windows, Mac, Linux.

Compile and install this in a RaspberryPi or any server of your choice in your cameras' local network, it will:

- stablish RTSP (TCP) RTP/RTPC (UDP) communications with your cameras.
- optionally, will record your streams on disk, deleting old files after a treshhold.
- optionally, will serve your real-time and recorded streams via HTTP/S server for remote viewing.

If you pair this service with nbplayer, you can stream to one or multiple TVs or monitors to render grids of your cameras.

![Running on a raspberry-pi-4-B and a 1080p-tv!](_photo-nbplayer-tv-1080.jpg ", raspberry-pi-4-B, nbplayer and a 1080p-tv")

# How to compile

For simplicity, create this folder structure:

- my_folder
   - sys-thinstream-src<br/>
      - [sys-thinstream-src](https://github.com/marcosjom/sys-thinstream-src)<br/>
   - sys-nbframework<br/>
      - [sys-nbframework-src](https://github.com/marcosjom/sys-nbframework-src)<br/>

You can create your own folders structure but it will require to update some paths in the projects and scripts.

The following steps will create and executable file.

## Windows

Open `projects/visual-studio/thinstream.sln` and compile the desired target.

## MacOS

Open `projects/xcode/thinstream.xcworkspace` and compile the desired target.

## Linux and Others

In a terminal:

```
cd sys-thinstream-src
make thinstream-server
```

Check each project's `Makefile` and `MakefileProject.mk` files, and the [MakefileFuncs.mk](https://github.com/marcosjom/sys-nbframework-src?tab=readme-ov-file#makefilefuncsmk) to understand the `make` process, including the accepted flags and targets.

# How to run

    ./thinstream-server [params]

Parameters:

    Global options

    -c [cfg-file]         : defines the configuration file to load
    -runServer            : runs the web-server until an interruption signal

    Certificate options
    
    -genCA                 : generates the root certificate and private key with the configuration file in '-c'
    -extendCA              : extends the root certificate and private key with the configuration file in '-c'
    -genSSL                : generates the SSL certificate per port with the configuration file in file in '-c'
        
    Debug options

    -msRunAndQuit num      : millisecs after starting to automatically activate stop-flag and exit, for debug and test
    -msSleepAfterRun num   : millisecs to sleep before exiting the main() funcion, for memory leak detection
    -msSleepBeforeRun num  : millisecs to sleep before running, for memory leak detection delay

Examples:

```
./thinstream-server -c myfile.json
./thinstream-server -c myfile.json -genCA -genSSL
./thinstream-server -c myfile.json -runServer
```
    
The first command loads and verifies the configuration file without running the server.
The second command creates the self-signed SSL certificates with `myfile.json`.
The third command runs the server using the configuration from `myfile.json`.

# Configuration file example

```
{
    "context": {
        "threads": {
            "amount": 1
            , "polling": {
                "timeout": 5
            }
        }
        , "filesystem": {
            "rootFolder": "thinstreamd"
            , "streams": {
                "rootPath": "folder_path_to_storage"
                , "cfgFilename": "folder.cfg.json"
                , "defaults": {
                    "limit": "10GB"
                    , "isReadOnly": false
                    , "keepUnknownFiles": false
                    , "waitForLoad": true
                    , "initialLoadThreads": 8
                }
            }
            , "pkgs": [ ]
        }
        , "cypher": {
            "enabled": true
            , "passLen": 20
            , "keysCacheSz": 32
            , "jobThreads": 3
            , "aes": {
                "salt": "my_cypher_salt"
                , "iterations": 80
            }
        }
        , "locale": {
            "paths": [ "my_locale.json" ]
        }
        , "deviceId": {
            "subject": [
                { "type": "C", "value": "NI" }
                , { "type": "ST", "value": "Managua" }
                , { "type": "L", "value": "Managua" }
                , { "type": "CN", "value": "Thin Stream Device" }
                , { "type": "O", "value": "Thin Stream Org" }
                , { "type": "OU", "value": "Thin Stream" }
            ]
            , "bits": 2048
            , "daysValid": 365
            , "keypass": "thinstream_dev_pass"
            , "keyname": "thinstream_dev"
            , "keypath": "thinstream_dev"
        }
        , "identityId": {
            "subject": [
                { "type": "C", "value": "NI" }
                , { "type": "ST", "value": "Managua" }
                , { "type": "L", "value": "Managua" }
                , { "type": "CN", "value": "Thin Stream Identity" }
                , { "type": "O", "value": "Thin Stream Org" }
                , { "type": "OU", "value": "Thin Stream" }
            ]
            , "bits": 2048
            , "daysValid": 365
            , "keypass": "thinstream_id_pass"
            , "keyname": "thinstream_id"
            , "keypath": "thinstream_id"
        }
        , "usrCert": {
            "subject": [
                { "type": "C", "value": "NI" }
                , { "type": "ST", "value": "Managua" }
                , { "type": "L", "value": "Managua" }
                , { "type": "CN", "value": "Thin Stream User" }
                , { "type": "O", "value": "Thin Stream Org" }
                , { "type": "OU", "value": "Thin Stream" }
            ]
            , "bits": 2048
            , "daysValid": 365
            , "keypass": "thinstream_pass"
            , "keyname": "thinstream_usr"
            , "keypath": "thinstream_usr.key.p12"
        }
        , "caCert": {
            "path": "thinstream_ca.cert.der"
            , "def": {
                "subject": [
                    { "type": "C", "value": "NI" }
                    , { "type": "ST", "value": "Managua" }
                    , { "type": "L", "value": "Managua" }
                    , { "type": "CN", "value": "Thin Stream CA" }
                    , { "type": "O", "value": "Thin Stream Org" }
                    , { "type": "OU", "value": "Thin Stream" }
                ]
                , "bits": 2048
                , "daysValid": 365
            }
            , "key": {
                "pass": "thinstream_ca_pass"
                , "name": "thinstream_ca"
                , "path": "thinstream_ca.key.p12"
            }
        }
    }
    , "server": {
        "defaults": {
            "streams": {
                "params": {
                    "read": {
                        "mode": "allways"
                        , "source": "memoryBuffer"
                    }
                    , "write": {
                        "mode": "allways"
                        , "files": {
                            "nameDigits": 6
                            , "buffKeyframes": false
                            , "index": {
                                "allNALU": false
                            }
                            , "payload": {
                                "divider": "1h"
                                , "limit": "1GB"
                            }
                            , "debug_": {
                                "write": {
                                    "simulateOnly": true
                                    , "atomicFlush": false
                                }
                            }
                        }
                    }
                    , "buffers": {
                        "units": {
                            "minAlloc": 64
                            , "maxKeep": 128
                        }
                    }
                    , "rtsp": {
                        "ports": {
                            "rtp": 0
                            , "rtcp": 0
                        }
                    }
                    , "rtp": {
                        "assumeConnDroppedAfter": "60s"
                    }
                }
                , "merger": {
                    "secsWaitAllFmtsMax___": 5
                }
            }
        }
        , "buffers": {
            "maxKeep": "64MB"
            , "atomicStats": false
        }
        , "rtsp": {
            "isDisabled" : false
            , "requests": {
                "timeouts": {
                    "min": {
                        "retries": 0
                        , "msConnect": 500
                    }
                    , "max": {
                        "retries": 1
                        , "msConnect": 2000
                    }
                }
            }
            , "rtp": {
                "isDisabled" : false
                , "server": {
                    "ports": [ 8000 ]
                    , "atomicStats": false
                    , "packets": {
                        "maxSize": "1536B"
                        , "initAlloc": "32MB"
                    }
                    , "perPort": {
                        "packetsBuffSz": 64
                        , "allocBuffSz": "2MB"
                    }
                    , "perStream": {
                        "packetsBuffSz": 128
                        , "orderingQueueSz": 16
                    }
                    , "debug_": {
                        "packetLostSimDiv": 0
                        , "doNotNotify": false
                    }
                }
            }
            , "rtcp": {
                "isDisabled" : false
                , "port": 8001
            }
        }
        , "telemetry": {
            "filesystem": {
                "statsLevel": "warning"
            }
            , "buffers": {
                "statsLevel": "info"
            }
            , "streams": {
                "statsLevel": "warning"
                , "perPortDetails": false
                , "perStreamDetails": false
                , "rtsp": {
                    "statsLevel": "warning"
                    , "rtp": {
                        "statsLevel": "info"
                    }
                    , "rtcp": {
                        "statsLevel": "warning"
                    }
                }
            }
            , "process": {
                "statsLevel": "info"
                , "locksByMethod": false
                , "threads_": [
                    {
                        "name": "tsThread"
                        , "firstKnownFunc": "TSThread_executeAndReleaseThread_"
                        , "statsLevel": "info"
                        , "locksByMethod": false
                    }
                ]
            }
        }
        , "streamsGrps": [
            {
                "isDisabled": false
                , "uid": "my_main_location_cams"
                , "name": "My Main Location Cams"
                , "streams": [
                    {
                        "isDisabled": false
                        , "device": {
                            "uid": "cam-door", "name": "Cam. Door"
                            , "server": "111.222.111.222", "port": "554", "useSSL": false
                            , "user": "admin", "pass": "my_cam_pass"
                        }
                        , "versions": [
                            {
                                "sid": "1-high", "isDisabled": false
                                , "uri": "rtsp://111.222.111.222/media/video1"
                                , "params": null
                            }
                            , {
                                "sid": "2-mid", "isDisabled": false
                                , "uri": "rtsp://111.222.111.222/media/video2"
                                , "params": null
                            }
                        ]
                    }
                    , {
                        "isDisabled": true
                        , "device": {
                            "uid": "cam-garage", "name": "Cam. Garage"
                            , "server": "111.222.111.223", "port": "554", "useSSL": false
                            , "user": "admin", "pass": "my_cam_pass"
                        }
                        , "versions": [
                            {
                                "sid": "1-high", "isDisabled": false
                                , "uri": "rtsp://111.222.111.223/Streaming/channels/101/trackID=1"
                                , "params": null
                            }
                            , {
                                "sid": "2-mid", "isDisabled": false
                                , "uri": "rtsp://111.222.111.223/Streaming/channels/102/trackID=1"
                                , "params": null
                            }
                        ]
                    }
                ]
            }
        ]
    }
    , "http": {
        "maxs": {
            "connsIn": 0
            , "secsIdle": 0
            , "bps" : { "in": 0, "out": 0 }
        }
        , "ports": [
            {
                "isDisabled": false
                , "port": 30443
                , "desc": "ALL REQUESTS CHANNEL"
                , "conns": {
                    "conns": 0
                    , "bps": { "in": 0, "out": 0 }
                }
                , "perConn": {
                    "recv": {
                        "buffSz": 0
                        , "bodyChunkSz": 0
                    }
                    , "sendQueue": {
                        "initialSz": 0
                        ,  "growthSz": 262144
                    }
                    , "limits": {
                        "secsIdle": 30
                        , "secsOverqueueing": 15
                        , "bps" : { "in": 0, "out": 0 }
                    }
                }
                , "ssl": {
                    "isDisabled": true
                    , "cert": {
                        "isRequested": true
                        , "isRequired": false
                        , "source": {
                            "path": "thinstream_channel_public.cert.der"
                            , "def": {
                                "subject": [
                                    { "type": "C", "value": "NI" }
                                    , { "type": "ST", "value": "Managua" }
                                    , { "type": "L", "value": "Managua" }
                                    , { "type": "CN", "value": "public.thinstream" }
                                    , { "type": "O", "value": "Thin Stream Org" }
                                    , { "type": "OU", "value": "Thin Stream" }
                                ]
                                , "bits": 2048
                                , "daysValid": 365
                            }
                            , "key": {
                                "pass": "my_ssl_cert_pass"
                                , "name": "my_ssl_cert"
                                , "path": "my_ssl_cert.key.p12"
                            }
                        }
                    }
                }
            }
        ]
    }
}

```

In this example one `streamsGrps` was defined with two `cameras`, and two `streams` qualities per camera.

The video-streams are saved in a local-storage folder up to 10GB, once the limit is reached old files will be deleted.

It is running a local HTTPS webserve in the 30443 `port` and the live or stored streams are served to external viewers.

One thread is used for the whole load.

# Contact

Visit [mortegam.com](https://mortegam.com/) to see other projects.

May you be surrounded by passionate and curious people. :-)



