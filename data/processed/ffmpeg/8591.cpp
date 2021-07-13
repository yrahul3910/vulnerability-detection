void *av_tree_insert(AVTreeNode **tp, void *key, int (*cmp)(void *key, const void *b), AVTreeNode **next){

    AVTreeNode *t= *tp;

    if(t){

        unsigned int v= cmp(t->elem, key);

        void *ret;

        if(!v){

            if(*next)

                return t->elem;

            else if(t->child[0]||t->child[1]){

                int i= !t->child[0];

                void *next_elem[2];

                av_tree_find(t->child[i], key, cmp, next_elem);

                key= t->elem= next_elem[i];

                v= -i;

            }else{

                *next= t;

                *tp=NULL;

                return NULL;

            }

        }

        ret= av_tree_insert(&t->child[v>>31], key, cmp, next);

        if(!ret){

            int i= (v>>31) ^ !!*next;

            AVTreeNode **child= &t->child[i];

            t->state += 2*i - 1;



            if(!(t->state&1)){

                if(t->state){

                    /* The following code is equivalent to

                    if((*child)->state*2 == -t->state)

                        rotate(child, i^1);

                    rotate(tp, i);



                    with rotate():

                    static void rotate(AVTreeNode **tp, int i){

                        AVTreeNode *t= *tp;



                        *tp= t->child[i];

                        t->child[i]= t->child[i]->child[i^1];

                        (*tp)->child[i^1]= t;

                        i= 4*t->state + 2*(*tp)->state + 12;

                          t  ->state=                     ((0x614586 >> i) & 3)-1;

                        (*tp)->state= ((*tp)->state>>1) + ((0x400EEA >> i) & 3)-1;

                    }

                    but such a rotate function is both bigger and slower

                    */

                    if((*child)->state*2 == -t->state){

                        *tp= (*child)->child[i^1];

                        (*child)->child[i^1]= (*tp)->child[i];

                        (*tp)->child[i]= *child;

                        *child= (*tp)->child[i^1];

                        (*tp)->child[i^1]= t;



                        (*tp)->child[0]->state= -((*tp)->state>0);

                        (*tp)->child[1]->state=   (*tp)->state<0 ;

                        (*tp)->state=0;

                    }else{

                        *tp= *child;

                        *child= (*child)->child[i^1];

                        (*tp)->child[i^1]= t;

                        if((*tp)->state) t->state  = 0;

                        else             t->state>>= 1;

                        (*tp)->state= -t->state;

                    }

                }

            }

            if(!(*tp)->state ^ !!*next)

                return key;

        }

        return ret;

    }else{

        *tp= *next; *next= NULL;

        (*tp)->elem= key;

        return NULL;

    }

}
