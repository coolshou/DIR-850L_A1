<module>
	<service><?=$GETCFG_SVC?></service>	
	<runtime>
		<services>
			<igmpproxy>												
				<?echo dump(2, "/runtime/services/igmpproxy");?>
			</igmpproxy>
		</services>
	</runtime>
	<SETCFG>ignore</SETCFG>
	<FATLADY>ignore</FATLADY>
	<ACTIVATE>ignore</ACTIVATE>			
</module>
