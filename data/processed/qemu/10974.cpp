static void cpu_notify_map_clients(void)

{

    MapClient *client;



    while (!LIST_EMPTY(&map_client_list)) {

        client = LIST_FIRST(&map_client_list);

        client->callback(client->opaque);

        LIST_REMOVE(client, link);

    }

}
