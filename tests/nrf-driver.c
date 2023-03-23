#include "nrf.h"
#include "nrf-hw-support.h"

// enable crc, enable 2 byte
#   define set_bit(x) (1<<(x))
#   define enable_crc      set_bit(3)
#   define crc_two_byte    set_bit(2)
#   define mask_int         set_bit(6)|set_bit(5)|set_bit(4)
enum {
    // pre-computed: can write into NRF_CONFIG to enable TX.
    tx_config = enable_crc | crc_two_byte | set_bit(PWR_UP) | mask_int,
    // pre-computed: can write into NRF_CONFIG to enable RX.
    rx_config = tx_config  | set_bit(PRIM_RX)
} ;


// Many settings on p57
// Some specifications on p23
nrf_t * nrf_init(nrf_conf_t c, uint32_t rxaddr, unsigned acked_p) {
    // nrf_t *n_s = staff_nrf_init(c, rxaddr, acked_p);
    // return n_s;

    // Allocate space
    nrf_t *n = kmalloc(sizeof(nrf_t));
    n->config = c;
    nrf_stat_start(n);
    n->spi = pin_init(c.ce_pin, c.spi_chip);
    n->rxaddr = rxaddr;


    // set CE pin
    // nrf_put8(n, NRF_CONFIG, 0b1111111); // PWR_UP=1, PRIM_RX=1, CRC=1 EN_CRC=1 MASK_RX_DR==1, MASK_TX_DS=1, MASK_MAX_RT=1
    nrf_put8(n, NRF_CONFIG, rx_config);

    // Enable pipe(s)
    // (don't) set auto ack
    // set up auto retransmission
    if (acked_p) {
        nrf_put8(n, NRF_EN_RXADDR, 0b11);
        nrf_put8(n, NRF_EN_AA, 0b11);
        nrf_put8(n, NRF_SETUP_RETR, 0b1110110); 
    } else {
        nrf_put8(n, NRF_EN_RXADDR, 0b10);
        nrf_put8(n, NRF_EN_AA, 0b00);
        nrf_put8(n, NRF_SETUP_RETR, 0b0);
    }



    // set up address widths
    nrf_put8(n, NRF_SETUP_AW, 0b01);

    // set up RF channel
    nrf_put8(n, NRF_RF_CH, c.channel);

    // set up RF set up register
    nrf_put8(n, NRF_RF_SETUP, 0b1110);

    // set up status register
    nrf_put8(n, NRF_STATUS, 0b1110);

    nrf_set_addr(n, NRF_TX_ADDR, 0, nrf_get_addr_nbytes(n));

    nrf_set_addr(n, NRF_RX_ADDR_P0, 0, nrf_get_addr_nbytes(n));
    nrf_put8(n, NRF_RX_PW_P0, c.nbytes);

    
    // nrf_putn(n, NRF_RX_ADDR_P1, &rxaddr, addr_nbytes);
    nrf_set_addr(n, NRF_RX_ADDR_P1, rxaddr, nrf_get_addr_nbytes(n));
    nrf_put8(n, NRF_RX_PW_P1, c.nbytes);


    // set up FIFO status register
    nrf_put8(n, NRF_FIFO_STATUS, 0b10001);

    nrf_rx_flush(n);
    nrf_tx_flush(n);

    // Put chip in power down mode
    nrf_set_pwrup_off(n);

    // power on
    nrf_set_pwrup_on(n);
    delay_ms(2);


    // set CE pin high
    gpio_set_on(c.ce_pin);   


    // nrf_rx_flush(n);
    // nrf_tx_flush(n);

    // assert(!nrf_tx_fifo_full(n));
    // assert(nrf_tx_fifo_empty(n));
    // assert(!nrf_rx_fifo_full(n));
    // assert(nrf_rx_fifo_empty(n));

    // assert(!nrf_has_rx_intr(n));
    // assert(!nrf_has_tx_intr(n));
    // assert(pipeid_empty(nrf_rx_get_pipeid(n)));
    // assert(!nrf_rx_has_packet(n));


    // nrf_dump("After my init", n);


    // should be true after setup.
    if(acked_p) {
        nrf_opt_assert(n, nrf_get8(n, NRF_CONFIG) == rx_config);
        nrf_opt_assert(n, nrf_pipe_is_enabled(n, 0));
        nrf_opt_assert(n, nrf_pipe_is_enabled(n, 1));
        nrf_opt_assert(n, nrf_pipe_is_acked(n, 0));
        nrf_opt_assert(n, nrf_pipe_is_acked(n, 1));
        nrf_opt_assert(n, nrf_tx_fifo_empty(n));
    } else {
        nrf_opt_assert(n, nrf_get8(n, NRF_CONFIG) == rx_config);
        nrf_opt_assert(n, !nrf_pipe_is_enabled(n, 0));
        nrf_opt_assert(n, nrf_pipe_is_enabled(n, 1));
        nrf_opt_assert(n, !nrf_pipe_is_acked(n, 1));
        nrf_opt_assert(n, nrf_tx_fifo_empty(n));
    }
    return n;
}

int nrf_tx_send_ack(nrf_t *n, uint32_t txaddr, 
    const void *msg, unsigned nbytes) {

    // default config for acked state.
    nrf_opt_assert(n, nrf_get8(n, NRF_CONFIG) == rx_config);
    nrf_opt_assert(n, nrf_pipe_is_enabled(n, 0));
    nrf_opt_assert(n, nrf_pipe_is_enabled(n, 1));
    nrf_opt_assert(n, nrf_pipe_is_acked(n, 0));
    nrf_opt_assert(n, nrf_pipe_is_acked(n, 1));
    nrf_opt_assert(n, nrf_tx_fifo_empty(n));

    // if interrupts not enabled: make sure we check for packets.
    // while(staff_nrf_get_pkts(n))
    //     ;
    while(nrf_get_pkts(n))
        ;
        

    // TODO: you would implement the rest of the code at this point.
    // int res_s = staff_nrf_tx_send_ack(n, txaddr, msg, nbytes);
    // return res_s;


    // p43


    // page 75
    // 1. Set the device to TX mode
    int res = 1;
    nrf_put8(n, NRF_CONFIG, tx_config);

    // 2. Set the TX address
    nrf_set_addr(n, NRF_TX_ADDR, txaddr, nrf_get_addr_nbytes(n));

    nrf_set_addr(n, NRF_RX_ADDR_P0, txaddr, nrf_get_addr_nbytes(n));

    // 3. Use nrf_putn and NRF_W_TX_PAYLOAD to write the message to the device
    nrf_putn(n, NRF_W_TX_PAYLOAD, msg, nbytes);

    // 4. High pulse the CE pin (p22 state diagram)
    // gpio_set_on(n->config.ce_pin);
    // delay_us(15);
    gpio_set_off(n->config.ce_pin);
    // delay_us(150); // additional delay needed
    delay_us(15);
    gpio_set_on(n->config.ce_pin);

    // 5. Wait until the TX fifo is empty
    while(!nrf_tx_fifo_empty(n)) {}

    if(nrf_has_tx_intr(n)) {
        // clear tx interrupt
        // nrf_tx_intr_clr(n);
        // break;
        res = nbytes;
    }

    // test this by setting retran to 1.
    if(nrf_has_max_rt_intr(n)) {
        // have to flush and clear the rt interrupt.
        nrf_dump("max inter config", n);
        panic("max intr\n");
    }
    

    // 6. Clear the TX interrupt. (write a 1 in the right position)
    // uint8_t cur_status = nrf_get8(n, NRF_STATUS);
    // uint8_t tx_ds = 1 << 5;
    // nrf_put8(n, NRF_STATUS, cur_status | tx_ds);
    nrf_tx_intr_clr(n);

    // 7. Set the device back in RX mode
    // gpio_set_off(n->config.ce_pin);
    nrf_put8(n, NRF_CONFIG, rx_config);
    // int res = nbytes; // what to grab for this?


    // tx interrupt better be cleared.
    nrf_opt_assert(n, !nrf_has_tx_intr(n));
    // better be back in rx mode.
    nrf_opt_assert(n, nrf_get8(n, NRF_CONFIG) == rx_config);
    return res;
}

int nrf_tx_send_noack(nrf_t *n, uint32_t txaddr, 
    const void *msg, unsigned nbytes) {

    // default state for no-ack config.
    nrf_opt_assert(n, nrf_get8(n, NRF_CONFIG) == rx_config);
    nrf_opt_assert(n, !nrf_pipe_is_enabled(n, 0));
    nrf_opt_assert(n, nrf_pipe_is_enabled(n, 1));
    nrf_opt_assert(n, !nrf_pipe_is_acked(n, 1));
    nrf_opt_assert(n, nrf_tx_fifo_empty(n));

    // if interrupts not enabled: make sure we check for packets.
    // while(staff_nrf_get_pkts(n))
    //     ;
    while(nrf_get_pkts(n))
        ;


    // TODO: you would implement the code here.
    // int res_s = staff_nrf_tx_send_noack(n, txaddr, msg, nbytes);
    // return res_s;
    // trace((char *)msg);

    // page 75
    // 1. Set the device to TX mode
    nrf_put8(n, NRF_CONFIG, tx_config);

    // 2. Set the TX address
    nrf_set_addr(n, NRF_TX_ADDR, txaddr, nrf_get_addr_nbytes(n));

    // 3. Use nrf_putn and NRF_W_TX_PAYLOAD to write the message to the device
    nrf_putn(n, NRF_W_TX_PAYLOAD, msg, nbytes);

    // 4. High pulse the CE pin (p22 state diagram)
    gpio_set_on(n->config.ce_pin);
    delay_us(15);
    gpio_set_off(n->config.ce_pin);
    delay_us(150); // additional delay needed
    gpio_set_on(n->config.ce_pin);

    // 5. Wait until the TX fifo is empty
    while(!nrf_tx_fifo_empty(n));

    // 6. Clear the TX interrupt. (write a 1 in the right position)
    uint8_t cur_status = nrf_get8(n, NRF_STATUS);
    uint8_t tx_ds = 1 << 5;
    nrf_put8(n, NRF_STATUS, cur_status | tx_ds);

    // 7. Set the device back in RX mode
    gpio_set_off(n->config.ce_pin);
    nrf_put8(n, NRF_CONFIG, rx_config);
    int res = nbytes; // what to grab for this?


    // tx interrupt better be cleared.
    nrf_opt_assert(n, !nrf_has_tx_intr(n));
    // better be back in rx mode.
    nrf_opt_assert(n, nrf_get8(n, NRF_CONFIG) == rx_config);
    return res;
}

int nrf_get_pkts(nrf_t *n) {
    nrf_opt_assert(n, nrf_get8(n, NRF_CONFIG) == rx_config);

    // TODO:
    // data sheet gives the sequence to follow to get packets.
    // p63: 
    //    1. read packet through spi.
    //    2. clear IRQ.
    //    3. read fifo status to see if more packets: 
    //       if so, repeat from (1) --- we need to do this now in case
    //       a packet arrives b/n (1) and (2)
    // done when: nrf_rx_fifo_empty(n)

    // int res_s = staff_nrf_get_pkts(n);
    // return res_s;
    // trace("res: %d\n", res);

    int res = 0;
    // trace("Starting\n");

    while(!nrf_rx_fifo_empty(n)) {
        // trace("In loop\n");
        uint8_t packet[64];
        nrf_getn(n, NRF_R_RX_PAYLOAD, packet, n->config.nbytes);
        cq_push_n(&n->recvq, packet, n->config.nbytes);
        res += n->config.nbytes;

        // clear interrupt
        nrf_rx_intr_clr(n);
    }


    nrf_opt_assert(n, nrf_get8(n, NRF_CONFIG) == rx_config);
    return res;
}
