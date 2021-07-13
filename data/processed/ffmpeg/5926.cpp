static int inject_fake_duration_metadata(RTMPContext *rt)

{

    // We need to insert the metdata packet directly after the FLV

    // header, i.e. we need to move all other already read data by the

    // size of our fake metadata packet.



    uint8_t* p;

    // Keep old flv_data pointer

    uint8_t* old_flv_data = rt->flv_data;

    // Allocate a new flv_data pointer with enough space for the additional package

    if (!(rt->flv_data = av_malloc(rt->flv_size + 55))) {

        rt->flv_data = old_flv_data;

        return AVERROR(ENOMEM);

    }



    // Copy FLV header

    memcpy(rt->flv_data, old_flv_data, 13);

    // Copy remaining packets

    memcpy(rt->flv_data + 13 + 55, old_flv_data + 13, rt->flv_size - 13);

    // Increase the size by the injected packet

    rt->flv_size += 55;

    // Delete the old FLV data

    av_free(old_flv_data);



    p = rt->flv_data + 13;

    bytestream_put_byte(&p, FLV_TAG_TYPE_META);

    bytestream_put_be24(&p, 40); // size of data part (sum of all parts below)

    bytestream_put_be24(&p, 0);  // timestamp

    bytestream_put_be32(&p, 0);  // reserved



    // first event name as a string

    bytestream_put_byte(&p, AMF_DATA_TYPE_STRING);

    // "onMetaData" as AMF string

    bytestream_put_be16(&p, 10);

    bytestream_put_buffer(&p, "onMetaData", 10);



    // mixed array (hash) with size and string/type/data tuples

    bytestream_put_byte(&p, AMF_DATA_TYPE_MIXEDARRAY);

    bytestream_put_be32(&p, 1); // metadata_count



    // "duration" as AMF string

    bytestream_put_be16(&p, 8);

    bytestream_put_buffer(&p, "duration", 8);

    bytestream_put_byte(&p, AMF_DATA_TYPE_NUMBER);

    bytestream_put_be64(&p, av_double2int(rt->duration));



    // Finalise object

    bytestream_put_be16(&p, 0); // Empty string

    bytestream_put_byte(&p, AMF_END_OF_OBJECT);

    bytestream_put_be32(&p, 40); // size of data part (sum of all parts below)



    return 0;

}
