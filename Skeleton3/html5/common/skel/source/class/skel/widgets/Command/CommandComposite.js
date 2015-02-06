/**
 * Base class for commands that serve as containers for other commands.
 */
/*global mImport */
/*******************************************************************************
 * @ignore( mImport)
 ******************************************************************************/

qx.Class.define("skel.widgets.Command.CommandComposite", {
    extend : skel.widgets.Command.Command,
    
    /**
     * Constructor
     */
    construct : function( label ) {
        this.base( arguments, label, null );
    },
    
    members : {
        
        /**
         * Returns a string identifying this command as a composite.
         * @return {String} a string indicating this command is a composite.
         */
        getType : function(){
            return skel.widgets.Command.Command.TYPE_COMPOSITE;
        },
        
        isVisibleMenu : function(){
            var menuVisible = true;
            if ( this.m_cmds !== null ){
                for ( var i = 0; i < this.m_cmds.length; i++ ){
                    menuVisible = this.m_cmds[i].isVisibleMenu();
                    if ( !menuVisible ){
                        break;
                    }
                }
            }
            return menuVisible;
        },
        
        isVisibleToolbar : function(){
            var toolVisible = true;
            if ( this.m_cmds !== null ){
                for ( var i = 0; i < this.m_cmds.length; i++ ){
                    toolVisible = this.m_cmds[i].isVisibleToolbar();
                    if ( !toolVisible ){
                        break;
                    }
                }
            }
            return toolVisible;
        },
       
        setVisibleMenu : function( visible){
            for ( var i = 0; i < this.m_cmds.length; i++ ){
                this.m_cmds[i].setVisibleMenu( visible );
            }
        },
        
        setVisibleToolbar : function( visible){
            for ( var i = 0; i < this.m_cmds.length; i++ ){
                this.m_cmds[i].setVisibleToolbar( visible );
            }
        },
        
        /**
         * Returns the command corresponding to the name if it matches the name of this
         * command or one of the children of this command; otherwise, returns null.
         * @param cmdName {String} the name of a command to look for.
         * @return {skel.widgets.Command.Command} the corresponding command or null if no matching command exists in the
         *       children of this object.
         */
        getCommand : function( cmdName ){
            var cmd = arguments.callee.base.apply(this, arguments, cmdName );
            if ( cmd === null && this.m_cmds !== null ){
                for ( var i = 0; i < this.m_cmds.length; i++ ){
                    cmd = this.m_cmds[i].getCommand( cmdName );
                    if ( cmd !== null ){
                        break;
                    }
                }
            }
            return cmd;
        },

        /**
         * Returns the child commands of this composite.
         * @return {Array} an array containing the immediate children of this command.
         */
        getValue : function(){
            return this.m_cmds;
        },
        
        m_cmds : null
        
    }
});