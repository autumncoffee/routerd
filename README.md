routerd
===

Microservice organizer.

Building
---

```
$ nix-build
```

Running
---

```
$ ./result/bin/routerd /path/to/config.json
```

Configuring
---

Simplest configuration file could look like that:

```
{
    "bind4": "127.0.0.1",
    "port": 1490,
    "hosts": {
        "output": [
            "127.0.0.1:14999"
        ]
    },
    "graphs": {
        "main": {
            "services": [
                "output"
            ]
        }
    },
    "routes": [
        {"r": "^/", "g": "main"}
    ]
}
```

`bind4` and `port` will tell routerd to listen for incoming requests on `127.0.0.1:1490`. There is also `bind6` which will tell `routerd` to listen on specified IPv6 address.

`hosts` contains the list of the addresses of microservices. In this example, service `output` is accessible via `127.0.0.1:14999`. If it had more than one instance ready to serve requests - address of second instance of `output` should've been added to the list.

`graphs` contains the list of microservice chains required to process the request. In this example, graph `main` lists only one service (`output`) to which the original request should be forwarded and which will generate the response that will be forwarded to the client. It is important to note that `output` is a special service name: routerd will only forward the response of service called `output` to the client, and won't do that with any other service.

`routes` contains the mapping between URI path and graph name that should be used for that path. In this example, graph `main` should be used for all pathes starting with `/`, effectively making graph `main` the default graph for all requests.

Services inside the graph also can depend on each other. Consider this:

```
{
    "bind4": "127.0.0.1",
    "port": 1490,
    "hosts": {
        "t1": [
            "127.0.0.1:10001"
        ],
        "t2": [
            "127.0.0.1:10002"
        ],
        "t3": [
            "127.0.0.1:10003"
        ],
        "t4": [
            "127.0.0.1:10004"
        ],
        "output": [
            "127.0.0.1:10005"
        ]
    },
    "graphs": {
        "main": {
            "services": [
                "t1",
                "t2",
                "t3",
                "t4",
                "output"
            ],
            "deps": [
                {"a": "output", "b": "t1"},
                {"a": "t4", "b": "output"},
                {"a": "t4", "b": "t2"},
                {"a": "t2", "b": "t1"}
            ]
        }
    },
    "routes": [
        {"r": "^/", "g": "main"}
    ]
}
```

In this example, graph `main` not only lists used services, but also provides dependency constraints for its services. `a` and `b` in dependency specification come from the phrase `service A depends on service B`. So:

1. service `output` depends on service `t1`;
2. service `t2` depends on service `t1` too;
3. service `t4` depends on services `output` and `t2`;
4. services `t1` and `t3` do not depend on any other service.

The word "depends" also could be read as "will be called after". So:

1. `t1` and `t3` will receive original client's request in parallel immediately after the request has been received by routerd;
2. after `t1` has responded to routerd, `output` and `t2` will receive original request + the response of `t1`, all in single HTTP request;
3. after service `output` has responded to routerd, its response will immediately be forwarded to the client;
4. after both `output` and `t2` have responded to routerd, `t4` will receive the original request + the responses of `output` and `t2` , all in single HTTP request;
5. the response of `t3` will be ignored because no other service depends on it.

Using
---

routerd is meant to forward HTTP requests and responses between the client and microservices.

Each request that routerd receives is transformed into multipart/form-data POST, looking exactly like file upload request, with the contents of the file named exactly the same as `X-AC-RouterD` header's value containing the body of original request.

All headers of the original request are preserved except the following:

1. `Content-Length`: it is now called `X-AC-RouterD-Content-Length` and contains the value of original `Content-Length` header; it also is present in the body part containing original request body with its original name;
2. `Content-Type`: it is now called `X-AC-RouterD-CType` and contains the value of original `Content-Type` header.

Since the request seen by microservices is now always HTTP POST, original request method could be found in `X-AC-RouterD-Method` header. Request method stored here is lowercased.

Note: if the original request is multipart/form-data POST itself - no exceptions are made and it becomes wrapped in another multipart/form-data POST like if it was any other request. So it is multipart/form-data inside multipart/form-data.
